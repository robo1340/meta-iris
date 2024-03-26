//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/resource.h>

#include <czmq.h>

#include "print_util.h"
#include "radio.h"
#include "gpio.h"
#include "state.h"
#include "turbo_wrapper.h"
#include "radio_events.h"
#include "bridge.h"
#include "beacon.h"

bool set_my_niceness(int niceness){
	printf("pid %u->%d\n", getpid(), niceness);
	return (setpriority(PRIO_PROCESS, 0, niceness) == 0);
}


int main(void) {
	setbuf(stdout, NULL);
	
	/*
	//handle Command Line args
	if (argc >= 3){ //handle args
		if (strcmp(argv[1],"-C")==0){ //use -C option to manually pass in the config path
			cfg_path = argv[2];
			ft_mode = true;
		}
	}	
	*/
	
	bridge_t * br = bridge_create(false, false, true);
	assert(br != NULL);
	
	//check radio part info
	radio_print_part_info();
	radio_print_func_info();
	
	//kick start the radio into receive mode
	radio_start_rx(br->my_state->current_channel, sizeof(coded_block_t));
	
	printf("entering loop, press ctrl+c to exit\n");
	
	//if (!set_my_niceness(-5)){
	//	printf("WARNING: set_my_niceness() failed!\n");
	//}
	
	while(!bridge_interrupted(br)) {
		if (!bridge_run(br)){
			printf("ERROR: bridge_run() failed\n");
		}
	}
	
	bridge_destroy(&br);
	return 0;
}



