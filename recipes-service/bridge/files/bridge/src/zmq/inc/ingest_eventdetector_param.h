/*
 * ingest_eventdetector_param.h
 *  Created on: 9/14/2023
 *      Author: c19358
 */

#ifndef ZMQ_SHARED_INC_INGEST_EVENTDETECTOR_PARAM_H_
#define ZMQ_SHARED_INC_INGEST_EVENTDETECTOR_PARAM_H_

#include <stdint.h>
#include <stdbool.h>

//the envelope format expected from the event detector
typedef struct __attribute__((__packed__)) event_detector_env_t_DEFINITION {
	uint8_t base_node;//should always be ENV_BOX_0
	uint8_t app_node; //should always be ENV_APP_EVENT
	uint8_t event_node; //should always be 0 - this is where the event detector places events
	uint8_t ff;       //should always be 0xff
	uint32_t event_id;//the parameter id
} event_detector_env_t;

bool ingest_eventdetector_param(uint8_t * env, size_t env_len , uint8_t * pay, size_t pay_len);

#endif 