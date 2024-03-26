#ifndef _BEACON_H
#define _BEACON_H
#endif 

#include "state.h"
#include "tlv_types.h"

#include <stdint.h>
#include <stdbool.h>

struct __attribute__((__packed__)) beacon_DEFINITION {
	uint8_t callsign[8];///<callsign of the beacon sender
	uint8_t ttl;		///<time to live of the beacon
	uint8_t opt;		///<beacon options
	uint8_t mac[6];		///<MAC of the beacon sender
	uint8_t ip[4];		///<ip of the beacon sender
	int64_t uptime;		///<uptime of the beacon sender at the time the beacon was sent
};

void beacon_init(beacon_t * beacon, state_t * state);

/** @brief handle a freshly received beacon and check if there was an IP address collision
 *  @param state 
 *  @param beacon the beacon received
 *  @return return true if the beacon was handled successfully, return false otherwise
 */
bool handle_beacon(state_t * state, beacon_t * beacon);

/** @brief decrement the ttl of a beacon
 *  @parmam beacon the beacon to decrement
 *  @return returns true if the beacon should be re-transmitted, returns false if it should be dropped
 */
bool beacon_ttl_decrement(beacon_t * beacon);




