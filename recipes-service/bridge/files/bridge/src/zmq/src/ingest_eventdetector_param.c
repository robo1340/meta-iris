/*
 * ingest_eventdetector_param.c
 *  Created on: 9/14/2023
 *      Author: c19358
 */
#include <cfg-util/cfg-util.h>
#include "ingest_eventdetector_param.h"
#include "polemos_if_public_data.h"
#include <czmq.h>

///<event detector message type that is published by the event detector,
//right now this is being copied from the event_detector code base since it isn't in a shared location
//line 202
//https://dev.azure.com/TextronAviation/AReS%203%20Software%20General/_git/meta-ares?path=/recipes-service/event-detector/files/event_detector/src/event_detector/event-detector/inc/state.h 
typedef struct __attribute__((__packed__)) event_detector_msg
{
	char event_tag[32];
	uint32_t module_uptime;
	int64_t unix_time;
	uint16_t state_id;
	uint16_t state_machine_id;
	uint8_t action_tag;
	uint8_t transition_tag;
	uint8_t module_uptime_valid;
	uint8_t buf2;
} event_detector_msg_t;

#define env_decoded(env_buf) ((event_detector_env_t*)env_buf)
#define pay_decoded(pay_buf) ((event_detector_msg_t*)pay_buf)

bool ingest_eventdetector_param(uint8_t * env, size_t env_len , uint8_t * pay, size_t pay_len) {
	//printf("ingest_eventdetector_param()\n");
	if (env_len != sizeof(event_detector_env_t)){ //check lengths of envelope
		//printf("env len check fail\n");
		return true; //still return true because we didn't fail, we just didn't get an event detector parameter
	}
	else if (env_decoded(env)->event_node != 0){ //check the event node on the event detector tree to make sure we are receiving an event parameter
		//printf("event node fail\n");
		return true; //still return true because we didn't fail, we just didn't get an event detector parameter
	}
	//else if (pay_len != sizeof(event_detector_msg_t)){ //check if the payload has a correct length
	//	//printf("payload len fail\n");
	//	return false; //now this is an error, we knew what payload length to expect for this envelope and didn't get it
	//}
	
	polemos_t * p = zhashx_first(polemos_controller_global->polemos_table);
	while (p != NULL){
		polemos_discrete_trigger_run(p, env_decoded(env)->event_id);
		p = zhashx_next(polemos_controller_global->polemos_table);
	}
	return true;
}