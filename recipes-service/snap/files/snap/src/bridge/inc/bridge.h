#ifndef bridge_H_
#define bridge_H_
 
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <czmq.h>
#include <zlib.h>

#include "state.h"
#include "composite_encoder.h"

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// bridge_t definition start////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

typedef struct bridge_t_DEFINITION bridge_t;
struct bridge_t_DEFINITION {
	state_t * my_state; ///<radio program state
	bool stopped;
	zpoller_t * poller;
	zsock_t * pub;
	zsock_t * sub;
	uint8_t * buf; //buffer the size of an uncoded payload
};

bridge_t * bridge_create(composite_encoder_t * encoder, uint8_t csma_rssi_threshold);

bool bridge_run(bridge_t * b);

bool bridge_interrupted(bridge_t * b);

void bridge_destroy(bridge_t ** to_destroy);

#endif
