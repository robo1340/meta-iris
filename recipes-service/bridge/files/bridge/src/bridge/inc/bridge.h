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

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// bridge_t definition start////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

typedef struct bridge_t_DEFINITION bridge_t;
struct bridge_t_DEFINITION {
	state_t * my_state; ///<radio program state
	bool stopped;
};

bridge_t * bridge_create(bool no_slip, bool no_gpio_events, bool no_incoming_socket);

bool bridge_slip_run(bridge_t * b);

bool bridge_run(bridge_t * b);

bool bridge_interrupted(bridge_t * b);

void bridge_destroy(bridge_t ** to_destroy);

#endif
