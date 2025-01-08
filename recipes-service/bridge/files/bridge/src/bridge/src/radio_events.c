/**
 * @file state.c
 * @author Haines Todd
 * @date 12/13/2022
 * @brief implementations of the objects the event detector engine uses to store its state
 */
 
#include "turbo_wrapper.h"
#include "timer.h"
#include "print_util.h" 

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <czmq.h>
#include <zlib.h>

//#include "radio_config.h"

#include "si446x_api_lib.h"
#include "si446x_patch.h" 
#include "radio.h"

//#define RADIO_EVENTS_DEBUG
#define RADIO_EVENTS_INFO
//#define RADIO_PRINT_STATE_CHANGE_INFO 
//#define RADIO_PRINT_TX_TIME
//#define RADIO_RECEIVE_PEDNING_INFO
//#define RADIO_SYNC_INFO
//#define RADIO_PREAMBE_INFO

//#define RADIO_COMMAND_ERROR_WARNING
//#define RADIO_INVALID_SYNC_WARNING

//#define HANDLING_TIMER


bool radio_preamble_detect_callback(state_t * state){
#ifdef RADIO_PREAMBE_INFO
	printf("INFO: preamble\n");

#endif
	if (state_transmitting(state)){
		printf("WARNING: received my own preamble\n");
		printf("radio->%s\n", radio_get_state_string(radio_get_modem_state()));
		//si446x_change_state(0x7); //change to transmitting state
		return true;
	}
	if (state_receiving(state)){ //this shouldn't happen
		return true;
		//printf("WARNING: throwing away radio frame mid reception\n");
	}
	if (!radio_frame_receive_start(state->receiving)) {return false;}
	//printf("[INTPT-P/S,PHDLR-P/S,MODEM-P/S,CHIP-P/S ]\n");
	//printArrHex((uint8_t*)&Si446xCmd.GET_INT_STATUS, 8);
	return true;
}

bool radio_sync_cb(state_t * state){
#ifdef RADIO_SYNC_INFO
	printf("INFO: sync\n");
#endif
	return true;
}

bool radio_receive_pending_callback(state_t * state){
#ifdef RADIO_RECEIVE_PEDNING_INFO
	printf("INFO: radio_receive_pending_callback()\n");
	if (!state_receiving(state)){
		printf("not receiving\n");
		return true;
	}

#endif 
	if (state_transmitting(state)) { //ignore if transmitting
		return true;
	}
	int remaining = radio_frame_bytes_remaining(state->receiving);
	if (remaining < 0){ return true;} //this was a false interrupt
	else if (remaining > RADIO_MAX_PACKET_LENGTH){
		printf("WARNING: %d bytes remain unfilled in radio_frame_t but frame is ended\n", remaining);
		//remaining = RADIO_MAX_PACKET_LENGTH;
		state_abort_transceiving(state);
		//return false;
		return true;
	}
	
	if (remaining != 0){
		si446x_read_rx_fifo(remaining, state->receiving->frame_position);
		state->receiving->frame_position += remaining;
	}

//#ifdef RADIO_EVENTS_INFO
//	remaining = radio_frame_bytes_remaining(state->receiving);
//	if (remaining == 0){
//		printf ("radio frame received\n");
//	}
//#endif

	
	packed_frame_t * pf = radio_frame_decode(state->receiving); //attempt to decode the received radio frame
	if (pf == NULL){
		printf("WARNING: radio_frame_decode() failed!\n");
		return true; //return true since this error was probably a CRC mismatch not an error caused by this function
	}
	
	frame_t * f = packed_frame_unpack(&pf); //unpack the radio frame
	if (f == NULL){
		printf("WARNING: packed_frame_unpack() failed!\n");
		return true;
	}
	
	if (!state_process_receive_frame(state, &f)){ //process the radio frame
		printf("WARNING: state_process_receive_frame() failed!\n");
		return false;
	}
	
	return true;	
}

bool radio_receive_fifo_almost_full_callback(state_t * state, uint32_t rx_almost_full_threshhold){
#ifdef RADIO_EVENTS_DEBUG
	printf("radio_receive_fifo_almost_full_callback()\n");
#endif
	int remaining = radio_frame_bytes_remaining(state->receiving);
	if (remaining < 0){ return true;}
	else if ((uint32_t)remaining >= rx_almost_full_threshhold){ // free space in the array is more than the threshold
		si446x_read_rx_fifo(rx_almost_full_threshhold, state->receiving->frame_position);
		state->receiving->frame_position += rx_almost_full_threshhold;
		
	}else{
		printf("ERROR: radio frame longer than radio_frame_t memory allocated\n");
		//si446x_read_rx_fifo(remaining, state->receiving->frame_position);
		//state->receiving->frame_position += remaining;
		state_abort_transceiving(state);
	}	
	return true;	
}

bool radio_transmit_complete_callback(state_t * state){
#ifdef RADIO_EVENTS_DEBUG
	printf("radio_transmit_complete_callback()\n");
#endif
	if (state->transmitting == NULL){
		//printf("WARNING: state->transmitting is NULL\n");
		//return false;
	} else {
		state->transmitting->frame_position = NULL; //set frame position to null for the transmitting frame
		radio_frame_destroy(&state->transmitting); //destroy the transmitted radio frame
	}
	
	timer_reset(&state->transmit_send_queue); 												//reset the timer to transmit another packet
	return true;	
}

bool radio_transmit_fifo_almost_full_callback(state_t * state, uint32_t tx_almost_full_treshhold){
#ifdef RADIO_EVENTS_DEBUG
	printf("DEBUG: radio_transmit_fifo_almost_full_callback()\n");
#endif
	int remaining = radio_frame_bytes_remaining(state->transmitting);
	if (remaining < 0){ return true;}
	else if((uint32_t)remaining > tx_almost_full_treshhold){ // Fill TX FIFO with the number of THRESHOLD bytes
		si446x_write_tx_fifo(tx_almost_full_treshhold, state->transmitting->frame_position);
		state->transmitting->frame_position += tx_almost_full_treshhold;
	}else{ // Fill TX FIFO with the number of remaining bytes
		si446x_write_tx_fifo(remaining, state->transmitting->frame_position);
		state->transmitting->frame_position += remaining;
	}
	return true;
}

bool radio_state_change_callback(state_t * state){
	
	uint8_t new_state = radio_get_modem_state();
#ifdef RADIO_PRINT_STATE_CHANGE_INFO
	printf("radio->%s\n", radio_get_state_string(new_state));	
#endif

#ifdef RADIO_PRINT_TX_TIME	
	static int tx_time_ms = -1;
	if (new_state == SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_TX){
		tx_time_ms = zclock_mono();
	}
	else if (tx_time_ms > 0){
		//printf("tx time %" PRId64 " ms\n",(zclock_mono()-tx_time_ms));
		printf("tx time %d ms\n",(int)(zclock_mono()-tx_time_ms));
		tx_time_ms = -1;
	}
#endif
	
	//if READY go to the receive state now
	if (new_state == SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY){
		//if (!radio_start_rx(state->current_channel, sizeof(coded_block_t))){return false;}
		if (!radio_start_rx(state->current_channel, state->receiving->frame_len)){return false;}
	}

	return true;
}

#define NO_INTERRUPT_PENDING ((Si446xCmd.GET_INT_STATUS.MODEM_PEND==0) && (Si446xCmd.GET_INT_STATUS.PH_PEND==0) && (Si446xCmd.GET_INT_STATUS.CHIP_PEND==0))
#define RSSI_LATCH					(Si446xCmd.GET_INT_STATUS.MODEM_PEND & 0x80)
#define POSTAMBLE_DETECT			(Si446xCmd.GET_INT_STATUS.MODEM_PEND & 0x40)
#define RSSI						(Si446xCmd.GET_INT_STATUS.MODEM_PEND & 0x08)
#define RSSI_JUMP_DETECTED 			(Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_RSSI_JUMP_BIT)
#define PREAMBLE_DETECTED			(Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_PREAMBLE_DETECT_BIT)
#define SYNC_DETECTED				(Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_SYNC_DETECT_BIT)
#define INVALID_PREAMBLE_DETECTED	(Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_INVALID_PREAMBLE_BIT)
#define FILLTER_MATCH 				(Si446xCmd.GET_INT_STATUS.PH_PEND & 0x80)
#define FILTER_MISS 				(Si446xCmd.GET_INT_STATUS.PH_PEND & 0x40)
#define PACKET_RX_COMPLETE			(Si446xCmd.GET_INT_STATUS.PH_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT)
#define RX_FIFO_ALMOST_FULL			(Si446xCmd.GET_INT_STATUS.PH_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_PH_STATUS_RX_FIFO_ALMOST_FULL_BIT)
#define PACKET_TX_COMPLETE			(Si446xCmd.GET_INT_STATUS.PH_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT)
#define TX_FIFO_ALMOST_EMPTY		(Si446xCmd.GET_INT_STATUS.PH_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_TX_FIFO_ALMOST_EMPTY_PEND_BIT)
#define FIFO_OVERFLOW_OR_UNDERFLOW	(Si446xCmd.GET_INT_STATUS.CHIP_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_CHIP_STATUS_FIFO_UNDERFLOW_OVERFLOW_ERROR_BIT)
#define STATE_CHANGE				(Si446xCmd.GET_INT_STATUS.CHIP_PEND 	& SI446X_CMD_GET_INT_STATUS_REP_CHIP_PEND_STATE_CHANGE_PEND_BIT)
#define COMMAND_ERROR				(Si446xCmd.GET_INT_STATUS.CHIP_PEND	& SI446X_CMD_GET_INT_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_BIT)
#define CHIP_READY					(Si446xCmd.GET_INT_STATUS.CHIP_PEND	& 0x04)
#define LOW_BAT						(Si446xCmd.GET_INT_STATUS.CHIP_PEND	& 0x02)
#define WAKE_UP_TIMER				(Si446xCmd.GET_INT_STATUS.CHIP_PEND	& 0x01)


#define INVALID_SYNC				(Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_ARG_MODEM_CLR_PEND_INVALID_SYNC_PEND_CLR_BIT)


/**
 * @brief callback function for when the radio module asserts the hardware interrupt signal
 * return false indicates error
 */
bool radio_event_callback(state_t * state) {
	//bool to_return;
#ifdef HANDLING_TIMER
	int64_t start = zclock_usecs();
#endif
	si446x_get_int_status(0u, 0u, 0u); // Read ITs, clear pending ones
	
	if (NO_INTERRUPT_PENDING){return true;}

#ifdef RADIO_EVENTS_DEBUG
	printf("[INTPT-P/S,PHDLR-P/S,MODEM-P/S,CHIP-P/S ]\n");
	printArrHex((uint8_t*)&Si446xCmd.GET_INT_STATUS, 8);
#endif
	
	bool handled = false;
	
	////////////////priority cases //////////////////////
	if (PREAMBLE_DETECTED){
		handled = true;
		if (!radio_preamble_detect_callback(state)) {return false;}
	}	
	
	if (SYNC_DETECTED){
		handled = true;
		if (!radio_sync_cb(state)) {return false;}
	}
	
	if(RX_FIFO_ALMOST_FULL){
		handled = true;
		if (!radio_receive_fifo_almost_full_callback(state, RADIO_RX_ALMOST_FULL_THRESHOLD)) {return false;}
#ifdef HANDLING_TIMER
		printf("handled %lld us at %lld\n", zclock_usecs()-start,zclock_usecs());
#endif
	}
	
	if (TX_FIFO_ALMOST_EMPTY){
		handled = true;
		if (!radio_transmit_fifo_almost_full_callback(state, RADIO_TX_ALMOST_EMPTY_THRESHOLD)) {return false;}
#ifdef HANDLING_TIMER
		printf("handled %lld us\n", zclock_usecs()-start);
#endif
	}

	////////////////receiving frame cases //////////////////////
	
	if (INVALID_PREAMBLE_DETECTED){
		handled = true;
	}
	
	if (PACKET_RX_COMPLETE){
		handled = true;
		if (!radio_receive_pending_callback(state)) {return false;}
	}
	
	////////////////transmitting frame cases //////////////////////
	if (PACKET_TX_COMPLETE){
		handled = true;
		if (!radio_transmit_complete_callback(state)) {return false;}
	}
	
	if (FIFO_OVERFLOW_OR_UNDERFLOW){
		handled = true;
		//printf("WARNING: TX/RX FIFO overflow detected! at %lld\n", zclock_usecs());
		//printf("WARNING: FIFO overflow detected! prem:%u, sync:%u\n", (PREAMBLE_DETECTED!=0),(SYNC_DETECTED!=0));
		if (state_receiving(state) && state_transmitting(state)){
			printf("wtf\n");
		}
		
		if (state_receiving(state)){
			int remaining = radio_frame_bytes_remaining(state->receiving);
			//if (remaining > 0){
			printf("WARNING: FIFO overflow while RECEIVING! %d/%u bytes remained\n", remaining, state->receiving->frame_len);
			state_abort_transceiving(state);
			//}
		}
		else if (state_transmitting(state)){
			printf("WARNING: FIFO overflow while TRANSMITTING!\n");
			state_abort_transceiving(state);
		}
		else {
			//ignore
			//printf("WARNING: FIFO overflow in non txrx state!\n");
		}
		
	}
	
	if (STATE_CHANGE){
		handled = true;
		if (!radio_state_change_callback(state)) {return false;}
	}
	
	if (COMMAND_ERROR){
		handled = true;
#ifdef RADIO_COMMAND_ERROR_WARNING
		printf("WARNING: radio_cmd_err_cb()\n");
		//printf("[INTPT-P/S,PHDLR-P/S,MODEM-P/S,CHIP-P/S ]\n");
		//printArrHex((uint8_t*)&Si446xCmd.GET_INT_STATUS, 8);
#endif
	}
	
	if (INVALID_SYNC){
		handled = true;
#ifdef RADIO_INVALID_SYNC_WARNING
		printf("WARNING: radio_invalid_sync_cb()\n");
#endif
	}
	
	if (RSSI_JUMP_DETECTED){
		//printf("RSSI_JUMP_DETECTED\n");
		handled = true;
	}
	
	if (FILLTER_MATCH){
		//printf("FILLTER_MATCH\n");
		handled = true;
	}
	
	if (FILTER_MISS){
		//printf("FILTER_MISS\n");
		handled = true;
	}
	
	if (RSSI_LATCH){
		//printf("RSSI_LATCH\n");
		handled = true;
	}
	
	if (RSSI){
		//printf("RSSI\n");
		//if (state_receiving(state)){printf("rx\n");}
		//else if (state_transmitting(state)){printf("tx\n");}
		handled = true;
	}
	
	if (POSTAMBLE_DETECT){
		//printf("POSTAMBLE_DETECT\n");
		handled = true;
	}
	
	if (CHIP_READY){
		//printf("CHIP_READY\n");
		handled = true;
	}
	
	if (LOW_BAT){
		//printf("LOW_BAT\n");
		handled = true;
	}
	
	if (WAKE_UP_TIMER){
		//printf("WAKE_UP_TIMER\n");
		handled = true;
	}
	
	if (!handled){
		printf("INFO: unhandled interrupt\n");
		printf("[INTPT-P/S,PHDLR-P/S,MODEM-P/S,CHIP-P/S ]\n");
		printArrHex((uint8_t*)&Si446xCmd.GET_INT_STATUS, 8);		
	}
	
	return true;
}
