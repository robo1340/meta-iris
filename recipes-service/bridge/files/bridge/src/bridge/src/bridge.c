//includes from DEPENDENCIES
#include "bridge.h"
#include "turbo_wrapper.h"
#include "tlv_types.h"
#include "print_util.h"
#include "gpio.h"
#include "radio_events.h"
#include "slip.h"
#include "serial.h"
#include "os_signal.h"
#include "radio.h"
#include "ipv4.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

//includes from the local src directory
#include "state.h"

//includes from libraries included in the Makefile's LIBS variable
#include <zmq.h>
#include <czmq.h>
#include <libserialport.h>

static uint8_t buf[1500]; //used by read_slip_port() and bridge_slip_run()

/** @brief read bytes from a slip port and return a pointer to a packet once an entire packet has been received
 *  @param slip_port open file descriptor to a serial port that is receiving slip data
 *  @return returns a pointer to a zchunk_t containing a packet on success, returns NULL on failure
 */
static zchunk_t * read_slip_port(int slip_port){
	int rc = 1;

	while (rc > 0){
		rc = serial_read(slip_port, buf, sizeof(buf));
		
		if (rc < 0){ //error
			printf("ERROR: sp_nonblocking_read() failed\n");
			return NULL;
		}
		else if (rc >= 0){//received something or received nothing but no error
			//if (rc > 0){
			//	printf("read_slip_port() -> %d\n", rc);
			//	printArrHex(buf,rc);
			//}
			zchunk_t * to_return = slip_decoder_feed(buf, rc);
			if (to_return != NULL){
				return to_return;
			}
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// bridge_t definition start////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bridge_t * bridge_create(bool no_slip, bool no_gpio_events, bool no_incoming_socket){
	printf("INFO: bridge_create()\n");
	if (!gpio_init()){
		printf("CRITICAL ERROR: gpio_init() failed!\n");
	}
	
	bridge_t * br = (bridge_t*)malloc(sizeof(bridge_t));
	if (br == NULL) {return NULL;}
	memset(br, 0, sizeof(bridge_t));
	br->stopped = false;
	
	srand(time(NULL));   // seed random number generator
	br->my_state = state();
	if (br->my_state == NULL){return NULL;}
	if (!state_restart_wifi_ap(br->my_state)){
		printf("WARNING: failed to start wifi AP!\n");
	}

	//init the turbo encoder
	//if (!turbo_wrapper_init(4)) {return NULL;}
	assert(br->my_state->encoder->frame_bytes_len==sizeof(packed_frame_t));
	

	return br;	
}

bool bridge_slip_run(bridge_t * b){
	static zchunk_t * packet;
	static uint32_t len;
	static bool low_priority;
	bool success;
	
	if (b->my_state->slip > 0){
		while(true){
			packet = read_slip_port(b->my_state->slip);
			if (packet == NULL) {break;}
			if (zchunk_size(packet) > MAX_PACKET_LEN){
				printf("WARNING: packet rejected, length exceeded maximum %d\n", MAX_PACKET_LEN);
				zchunk_destroy(&packet);
				return false;
			}
			
			printf("to tx: len %u ", zchunk_size(packet));
			ipv4_print((ipv4_hdr_t*)zchunk_data(packet));
			
			ipv4_set_ttl((ipv4_hdr_t*)zchunk_data(packet), b->my_state->ttl_set);
			len = sizeof(buf); //compress sets len so set it now
			low_priority = check_udp_dst_port((udp_datagram_t*)zchunk_data(packet), b->my_state->low_priority_udp_port);
			//printf("low priority %u\n", low_priority);
			if ((b->my_state->compress_ipv4) && (compress(buf, (uLongf*)&len, zchunk_data(packet), zchunk_size(packet)) == Z_OK )){
				if (!low_priority){
					success = state_append_loading_dock(b->my_state, TYPE_IPV4_COMPRESSED, len, buf);
				} else {
					success = state_append_low_priority_packet(b->my_state, TYPE_IPV4_COMPRESSED, len, buf);
				}
			} else {
				if (!low_priority){
					success = state_append_loading_dock(b->my_state, TYPE_IPV4, zchunk_size(packet), zchunk_data(packet));
				} else {
					success = state_append_low_priority_packet(b->my_state, TYPE_IPV4_COMPRESSED, len, buf);
				}
			}
			//printArrHex(zchunk_data(packet), zchunk_size(packet));
			zchunk_destroy(&packet);
			if (!success){
				printf("ERROR: state_append_loading_dock() failed!\n");
			}
			return success;
		}		
	}
	
	return true;
}

bool bridge_run(bridge_t * b){	
	if (gpio_poll()){
		if(!radio_event_callback(b->my_state)){
			printf("WARNING: radio_event_callback() failed\n");
		}
		return true;
	}
	else {
		state_run(b->my_state); //run regularly scheduled tasks
	}
	
	if (state_transceiving(b->my_state)) {
		return true;
	}
	else if (!bridge_slip_run(b)){
		printf("ERROR: bridge_slip_run() failed\n");
		return false;
	}
	return true;
}

bool bridge_interrupted(bridge_t * b){
	return ((os_interrupted!=0) || (b->stopped));
}

void bridge_destroy(bridge_t ** to_destroy){
    printf("destroying bridge_t at 0x%p\n", (void*)*to_destroy);
	
	printf("gpio_deinit\n");
	gpio_deinit();
	
	printf("turbo_wrapper_deinit\n");
	turbo_wrapper_deinit();
	
	state_destroy(&(*to_destroy)->my_state);

	free(*to_destroy);
	*to_destroy = NULL;
}
