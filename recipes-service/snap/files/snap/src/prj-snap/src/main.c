//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/resource.h>
#include <getopt.h>

#include <czmq.h>

#include "print_util.h"
#include "radio.h"
#include "gpio.h"
#include "state.h"
#include "radio_events.h"
#include "bridge.h"
#include "composite_encoder.h"

bool set_my_niceness(int niceness){
	printf("pid %u->%d\n", getpid(), niceness);
	return (setpriority(PRIO_PROCESS, 0, niceness) == 0);
}

// Flag for verbose output
int rs_mode = 2;
int payload_len=100;
int block_len = 20;

static void usage(void){
	printf("snap radio program\n");
	printf("args:\n");
	printf("--nrs      -n - disable reed solomon encoding\n");
	printf("--payload -p - specify the uncoded payload length (in bytes)\n");
	printf("--block   -b   - specify the reed solomon block length (in bytes)\n");
	exit(0);
}

int main(int argc, char **argv) {
	zclock_sleep(1000);
	setbuf(stdout, NULL);
	
	//handle command line arguments
	int c;

    // Define long options
    static struct option long_options[] = {
        // Options that set a flag
        {"disable-reed-solomon", required_argument, NULL, 'n'},
		{"help", no_argument, NULL, 'h'},
        //{"brief",   no_argument, &verbose_flag, 0},
        
        // Options that return a value
        //{"pay",     no_argument,       0, 'p'},
        {"payload",  required_argument, NULL, 'p'},
        {"block",  required_argument, NULL, 'b'},
        {0, 0, 0, 0} // End of options array
    };
	
	int option_index = 0; // Stores the index of the matched long option

    while (1) {
        // Call getopt_long to parse options
        // "abc:d:f:" specifies short options and their argument requirements:
        // 'a' and 'b' take no argument.
        // 'c', 'd', and 'f' require an argument.
        c = getopt_long(argc, argv, "nhp:b:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0: // For options that set a flag (like --verbose or --brief)
                if (long_options[option_index].flag != 0) {
                    printf("Option %s set flag %d\n", long_options[option_index].name, *long_options[option_index].flag);
                }
                break;
			case 'n':
				if (optarg != NULL){
					if (strcmp(optarg, "true")==0){
						rs_mode = 0;
					} else {
						rs_mode = 2;
					}
				}
				break;
            case 'p':
                if (optarg != NULL) {
					payload_len = atoi(optarg);
				}
                break;
            case 'b':
                if (optarg != NULL) {
					block_len = atoi(optarg);
				}
                break;
			case 'h':
				usage();
				break;
            case '?': // Unknown option
                // getopt_long already printed an error message
                break;
	
            default:
                printf("?? getopt_long returned character code 0%o ??\n", c);
        }
    }
	//printf("command line options %u, %u, %u\n", rs_mode, payload_len, block_len);
	//exit(0);
	
	composite_encoder_t * encoder = composite_encoder(payload_len, rs_mode, block_len, NULL);
	
	FILE *fptr;
    fptr = fopen("/tmp/packet_length.txt", "w");
    fprintf(fptr, "%d", encoder->uncoded_len);
    fclose(fptr);
	
	bridge_t * br = bridge_create(encoder);
	assert(br != NULL);
	
	//check radio part info
	radio_print_part_info();
	radio_print_func_info();
	
	//kick start the radio into receive mode
	radio_start_rx(br->my_state->current_channel, br->my_state->receiving->frame_len); 
	
	printf("entering loop, press ctrl+c to exit\n");
	
	//if (!set_my_niceness(-10)){
	//	printf("WARNING: set_my_niceness() failed!\n");
	//}
	
	while(!bridge_interrupted(br)) {
		if (!bridge_run(br)){
			printf("ERROR: bridge_run() failed\n");
		}
	}
	
	composite_encoder_destroy(&encoder);
	bridge_destroy(&br);
	return 0;
}




