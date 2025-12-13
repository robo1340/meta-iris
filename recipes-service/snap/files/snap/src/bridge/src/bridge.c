//includes from DEPENDENCIES
#include "bridge.h"
#include "turbo_wrapper.h"
#include "tlv_types.h"
#include "print_util.h"
#include "gpio.h"
#include "radio_events.h"
#include "os_signal.h"
#include "radio.h"

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

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// bridge_t definition start////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bridge_t * bridge_create(composite_encoder_t * encoder, uint8_t csma_rssi_threshold){
	printf("INFO: bridge_create()\n");
	if (!gpio_init()){
		printf("CRITICAL ERROR: gpio_init() failed!\n");
	}
	os_catch_signals();
	
	bridge_t * br = (bridge_t*)malloc(sizeof(bridge_t));
	if (br == NULL) {return NULL;}
	memset(br, 0, sizeof(bridge_t));
	br->stopped = false;

	//create zmq objects that will receive and send messages to local processes
	br->sub = zsock_new(ZMQ_PULL);
	//zsock_set_subscribe(br->sub,"");
	zsock_bind(br->sub, "ipc:///tmp/to_tx.ipc");
	
    br->pub = zsock_new_pub ("ipc:///tmp/to_rx.ipc");
	
	br->poller = zpoller_new (br->sub, NULL);
	br->buf = malloc(encoder->uncoded_len);
	memset(br->buf, 0, encoder->uncoded_len);
	
	srand(time(NULL));   // seed random number generator
	br->my_state = state(encoder, br->pub, csma_rssi_threshold);
	if (br->my_state == NULL){return NULL;}

	return br;	
}

bool bridge_internal_run(bridge_t * b){
	//printf("bridge_internal_run()\n");
	
	void * socket = zpoller_wait(b->poller, 0);
	
	if (zpoller_expired(b->poller)){
		//printf("poller expired\n");
		return true;
	}
	else if (zpoller_terminated(b->poller)){
		b->stopped = true;
		printf("poller terminated\n");
		return true;
	}
	if (socket == b->sub) { //a request is coming in from the frontend
		zmsg_t * msg = zmsg_recv(b->sub);
		zframe_t * pay = zmsg_pop(msg);
		
		uint32_t to_copy = zframe_size(pay);
		if (zframe_size(pay) > b->my_state->encoder->uncoded_len){
			printf("WARNING: message to send is length %u, greather than maximum %u will be truncated\n", zframe_size(pay), b->my_state->encoder->uncoded_len);
			to_copy = b->my_state->encoder->uncoded_len;
		}

		memset(b->buf, 0, b->my_state->encoder->uncoded_len);
		memcpy(b->buf, zframe_data(pay), to_copy);
		zframe_destroy(&pay);
		zmsg_destroy(&msg); 
		
		state_add_frame_to_send_queue(b->my_state, b->buf, b->my_state->encoder->uncoded_len);
		
		return true;
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
	else if (!bridge_internal_run(b)){
		printf("ERROR: bridge_internal_run() failed\n");
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
	
	state_destroy(&(*to_destroy)->my_state);

	zsock_destroy (&(*to_destroy)->pub);
    zsock_destroy (&(*to_destroy)->sub);
	zpoller_destroy(&(*to_destroy)->poller);

	free((*to_destroy)->buf);

	free(*to_destroy);

	*to_destroy = NULL;
}
