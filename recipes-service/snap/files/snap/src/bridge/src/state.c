//#include "turbo_wrapper.h"
#include "state.h"
#include "print_util.h"
#include "radio_events.h"
#include "timer.h"
#include "radio_hal.h" //radio_hal_init()
#include "tlv_types.h"
#include "avg.h" //exp_moving_avg(uint32_t new_sample);

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <czmq.h>
#include <zlib.h>
#include <libserialport.h>

#include "radio.h"
#include "csma.h"

#include <linux/if.h>
#include <linux/if_tun.h>


//reporting debug definitions
#define DEBUG
//#define OBJECT_DESTROY_DEBUG
//#define TX_START_DEBUG

state_t * global_state;

#define RADIO_OTHER_CONFIG_DIR "/snap/conf/si4463/other/"			
#define RADIO_MODEM_CONFIG_DIR "/snap/conf/si4463/modem/"			
#define RADIO_DEFAULT_CHANNEL                                0     ///<the default radio channel to use								  

static bool state_read_config_settings(state_t * state, char * path);

state_t * state(composite_encoder_t * encoder, zsock_t * pub){
	state_t * to_return = (state_t*)malloc(sizeof(state_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->si4463_load_order = zlistx_new();
	if (to_return->si4463_load_order == NULL) {return NULL;}
	zlistx_add_end(to_return->si4463_load_order, "RF_POWER_UP");

	//read the radio config settings 
	to_return->radio_config = radio_load_configs(RADIO_MODEM_CONFIG_DIR, NULL, RADIO_MODEM_CONFIG_REGEX);
	to_return->radio_config = radio_load_configs(RADIO_OTHER_CONFIG_DIR, to_return->radio_config, RADIO_CONFIG_REGEX);
	if (to_return->radio_config == NULL){
		printf("CRITICAL ERROR: could not load radio config settings\n");
		return NULL;
	}
	
	if (!radio_init(to_return->radio_config, to_return->si4463_load_order)){
		printf("ERROR: radio_init() failed!\n");
	}
	
	to_return->tx_backoff_min_ms = 10;
	to_return->current_channel = RADIO_DEFAULT_CHANNEL;
	//to_return->tx_backoff_min_ms = to_return->transmit_queue_period_ms - to_return->transmit_queue_variance_ms;
	
	////create a radio frame send queue
	to_return->send_queue = zlistx_new();
	if (to_return->send_queue == NULL) {return NULL;}
	zlistx_set_destructor(to_return->send_queue, (zlistx_destructor_fn*)radio_frame_destroy);
	
	timer_init(&to_return->transmit_send_queue, true,   10, 0);
	//timer_init(&to_return->transmit_send_queue, true,   to_return->transmit_queue_period_ms, to_return->transmit_queue_variance_ms);
	csma_init(&to_return->csma, 50, 5, 10); //probabillity to transmit, min backoff ms, max backoff ms
	
	to_return->encoder = encoder;
	to_return->pub = pub;
	
	to_return->transmitting = NULL;
	to_return->receiving	= radio_frame(to_return->encoder->coded_len);
	
	global_state = to_return; //set the global state object, this is ok since this is a singleton class
	
	composite_encoder_report(to_return->encoder);
	
	return to_return;
}

bool state_process_receive_frame(state_t * state, zframe_t ** to_rx, uint8_t latched_rssi){
	//printf("DEBUG: state_process_receive_frame(%u)\n", zframe_size(*to_rx));
	//printArrHex(zframe_data(*to_rx), zframe_size(*to_rx));
	
	zframe_t * rssi = zframe_new (&latched_rssi, 1);
	
	zframe_send(to_rx, state->pub, ZFRAME_MORE);
	zframe_send (&rssi, state->pub, 0);
	return true;
}

bool state_transmitting(state_t * state) {return (state->transmitting != NULL);}

bool state_receiving(state_t * state){
	return radio_frame_active(state->receiving);
}

bool state_transceiving(state_t * state){
	return (radio_frame_active(state->receiving) || (state->transmitting != NULL));
}

bool state_add_frame_to_send_queue(state_t * state, uint8_t * pay, uint32_t len){
	//printArrHex(pay, len);
	
	uint8_t * encoded = composite_encoder_encode(state->encoder, pay, len);
	if (encoded == NULL){
		printf("ERROR: state_add_frame_to_send_queue() failed!\n");
		return false;
	}
	radio_frame_t * to_tx = radio_frame(state->encoder->coded_len);
	memcpy(to_tx->frame_ptr, encoded, state->encoder->coded_len);
	
	//printArrHex(to_tx->frame_ptr, to_tx->frame_len);
	
	//printf("DEBUG: state_add_frame_to_send_queue(%u)\n", to_tx->frame_len);
	zlistx_add_end(state->send_queue, to_tx);
	return true;
}

bool state_peek_next_frame_to_transmit(state_t * state){
	return (zlistx_size(state->send_queue) > 0);
}

radio_frame_t * state_get_next_frame_to_transmit(state_t * state){
	//printf("DEBUG: state_get_next_frame_to_transmit()\n");
	radio_frame_t * to_return = zlistx_first(state->send_queue);
	state->transmitting = to_return;
	if (to_return != NULL){
		zlistx_detach_cur(state->send_queue);
	}
	return to_return;
}

void state_finish_receiving(state_t * state){
	radio_frame_reset(state->receiving);
	radio_start_rx(state->current_channel, state->receiving->frame_len);
}

void state_abort_transceiving(state_t * state){
	printf("INFO: state_abort_transceiving()\n");
	
	radio_frame_reset(state->receiving);
	if (state->transmitting != NULL){
		radio_frame_destroy(&state->transmitting);
	}
	radio_start_rx(state->current_channel, state->receiving->frame_len);
	//si446x_change_state(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY); //change to ready state
}

#define TXRX_TIMEOUT_MS 16000 ///<timeout value used to detect when the program is in a locked state

#define RSSI_THRESHHOLD 10 ///<only transmit if AVG RSSI plus this is lower than this

static void state_tx_now(state_t * state){
	state->transmitting = state_get_next_frame_to_transmit(state);
	if (state->transmitting != NULL){
		if (!radio_frame_transmit_start(state->transmitting, state->current_channel)){
			printf("ERROR: radio_frame_transmit_start() failed!\n");
		}
	} else {
		printf("ERROR: state_get_next_frame_to_transmit() returned NULL\n");
	}
}

void state_run(state_t * state){
	static int64_t txrx_timer = -1;
	//static task_timer_t radio_state_check = {false, 250, 0, 0};
	//static task_timer_t radio_rssi_check  = {false, 5, 0};
	static uint32_t rssi, avg_rssi;

	if (state_transceiving(state)) {
		radio_event_callback(state);
		if (txrx_timer < 0){
			txrx_timer = zclock_mono();
		}
		else if ((zclock_mono()-txrx_timer)>TXRX_TIMEOUT_MS){
			printf("timed out ");
			state_abort_transceiving(state);
			txrx_timer = -1;
		}
		return; //return immediately, when transmitting or receiving focus only on that
	} else if (txrx_timer > 0) {
		txrx_timer = -1;
	}
	
	//if (timer_run(&radio_rssi_check)){
	//	radio_print_modem_status(false);
	//}
	
	avg_rssi = exp_moving_avg(radio_get_rssi());
	
	if (csma_enabled(&state->csma)) { //ready to transmit but medium is busy, check it after random backoff then probabalitistically transmit
		if (csma_run_countdown(&state->csma)){
			rssi = radio_get_rssi();
			printf("waiting rssi %u\n", rssi);
			if (rssi < (avg_rssi+RSSI_THRESHHOLD)){
				if (csma_run_transmit(&state->csma)){ //transmit, csma will be disabled now
					state_tx_now(state);
				}			
			}
		}
	}
	else if (timer_run(&state->transmit_send_queue)) { //csma is not running start a transmission
		
		if (!state_peek_next_frame_to_transmit(state)){ //if there is nothing to transmit set back-off to its minimum
			state->transmit_send_queue.period_ms = state->tx_backoff_min_ms;
		} else { //there is a frame to transmit
			rssi = radio_get_rssi();
			printf("rssi %u\n", rssi);
			if (rssi < (avg_rssi+RSSI_THRESHHOLD)){
				state_tx_now(state);
			} else {
				csma_reset(&state->csma); //start the csma algorithm
			}			
		}
	}
	
	//if (timer_run(&radio_state_check)){
	//	radio_print_state(false);
	//}
	
}




void state_destroy(state_t ** to_destroy){
    printf("destroying state_t at 0x%p\n", (void*)*to_destroy);
	radio_hal_deinit();

	zlistx_destroy(&(*to_destroy)->send_queue);

	zlistx_destroy(&(*to_destroy)->si4463_load_order);
	zhashx_destroy(&(*to_destroy)->radio_config);	

	free(*to_destroy); //free the state_t from memory
	*to_destroy = NULL; //set the state_t* to be a NULL pointer
	return;	
}

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// uint16_ptr_t definition start///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

uint16_t * uint16_ptr(uint16_t val){
	uint16_t * to_return = (uint16_t*)malloc(sizeof(uint16_t));
	*to_return = val;
	return to_return;
}

void uint16_ptr_destroy(uint16_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying uint16_t at 0x%p\n", (void*)*to_destroy);
#endif
	free(*to_destroy);
	*to_destroy = NULL;
	return;		
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// tlv_t definition start///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

tlv_t * tlv(uint8_t type, uint16_t len, uint8_t * value){
	tlv_t * to_return = (tlv_t*)malloc(sizeof(tlv_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->type = type;
	to_return->len  = len;
	
	to_return->value = (uint8_t*)malloc(len);
	if (to_return->value == NULL) {
		free(to_return);
		return NULL;
	}
	if (value != NULL){
		memcpy(to_return->value, value, len);
	}
	return to_return;
}

int tlv_len(uint16_t len){
	return (sizeof(uint8_t) + sizeof(uint16_t) + len);
}

void tlv_destroy(tlv_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying tlv_t at 0x%p\n", (void*)*to_destroy);
#endif
	if ((*to_destroy)->value != NULL){
		free((*to_destroy)->value);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// radio_frame_t definition start////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

radio_frame_t * radio_frame(size_t len){
	radio_frame_t * to_return = (radio_frame_t*)malloc(sizeof(radio_frame_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->frame_ptr = (uint8_t*)malloc(len);
	if (to_return->frame_ptr == NULL) {return NULL;}
	to_return->frame_len = len;
	to_return->frame_position = NULL;
	memset(to_return->frame_ptr, 0, to_return->frame_len);
	return to_return;	
}

int radio_frame_bytes_remaining(radio_frame_t * rf){
	if ((rf == NULL) || (rf->frame_position == NULL)){
		return -1;
	} else {
		return (int)(rf->frame_len - (rf->frame_position - rf->frame_ptr));
	}
}

bool radio_frame_transmit_start(radio_frame_t * frame, uint8_t channel){
#ifdef TX_START_DEBUG
	printf("INFO: radio_frame_transmit_start(%u)\n", frame->frame_len);
#endif
	
	if (frame->frame_ptr == NULL) {return false;} //nothing to transmit
	
	frame->frame_position = frame->frame_ptr;
	
	si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT|SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT); // Reset TX FIFO
	si446x_get_int_status(0u, 0u, 0u); // Read ITs, clear pending ones

	// Fill the TX fifo with data
	if (frame->frame_len >= RADIO_MAX_PACKET_LENGTH){ // Data to be sent is more than the size of TX FIFO
		si446x_write_tx_fifo(RADIO_MAX_PACKET_LENGTH, frame->frame_position);
		frame->frame_position += RADIO_MAX_PACKET_LENGTH;
	}else{ // Data to be sent is less or equal than the size of TX FIFO
		si446x_write_tx_fifo(frame->frame_len, frame->frame_position);
		frame->frame_position += frame->frame_len;
	}
	
	//printf("start tx\n");
	
	// Start sending packet, channel 0, START immediately, Packet length of the frame
	si446x_start_tx(channel, 0x30,  frame->frame_len);
	
	return true;
}

bool radio_frame_receive_start(radio_frame_t * frame){
	if (frame->frame_ptr == NULL) {return false;} //no memory allocated to receive
	memset(frame->frame_ptr, 0, frame->frame_len);
	frame->frame_position = frame->frame_ptr;
	return true;
}

void radio_frame_reset(radio_frame_t * frame){
	frame->frame_position = NULL;
	memset(frame->frame_ptr,0,frame->frame_len);
}

void radio_frame_destroy(radio_frame_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying radio_frame_t at 0x%p\n", (void*)*to_destroy);
#endif
	if ((*to_destroy)->frame_ptr != NULL){
		free((*to_destroy)->frame_ptr);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}