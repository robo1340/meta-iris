//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <czmq.h>

#include "print_util.h"
#include "radio.h"
#include "gpio.h"
#include "state.h"
#include "turbo_wrapper.h"
#include "radio_events.h"
#include "bridge.h"
#include "beacon.h"

int main(void) {
	
	bridge_t * br = bridge_create(true, false, true);
	assert(br != NULL);
	
	//check radio part info
	radio_print_part_info();
	radio_print_func_info();
	
	beacon_t my_beacon;
	beacon_init(&my_beacon, br->my_state);
	if (!state_append_loading_dock(br->my_state, TYPE_BEACON, sizeof(beacon_t), (uint8_t*)&my_beacon)){
		printf("ERROR: state_append_loading_dock() failed!\n");
	}
	
	//kick start the radio into receive mode
	radio_start_rx(RADIO_DEFAULT_CHANNEL, sizeof(coded_block_t));
	
	printf("entering loop, press ctrl+c to exit\n");
	while(!bridge_interrupted(br)) {
		if (!bridge_zmq_run(br)){
			printf("ERROR: bridge_zmq_run() failed\n");
		}
	}
	
	bridge_destroy(&br);
	return 0;
}
