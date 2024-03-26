#include "beacon.h"
#include "print_util.h"

#include <stdio.h>
#include <string.h>
#include <czmq.h>


void beacon_init(beacon_t * beacon, state_t * state){
	memcpy(beacon->callsign, state->callsign, sizeof(beacon->callsign));
	beacon->ttl = DEFAULT_BEACON_TTL;
	beacon->opt = 0;
	memcpy(beacon->mac, state->mac, sizeof(beacon->mac));
	memcpy(beacon->ip,  state->ip,  sizeof(beacon->ip));
	beacon->uptime = zclock_mono();
}

bool handle_beacon(state_t * state, beacon_t * beacon){
	printf("INFO: handle_beacon(%.*s,ttl=%u)\n", sizeof(beacon->callsign), beacon->callsign, beacon->ttl);
	return true;
}

bool beacon_ttl_decrement(beacon_t * beacon){
	if (beacon->ttl == 0) {return false;}
	beacon->ttl--;
	return true;
}

