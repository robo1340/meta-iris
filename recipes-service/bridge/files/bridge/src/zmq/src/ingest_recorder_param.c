/*
 * ingest_recorder_param.c
 *
 *  Created on: Mar 3, 2022
 *      Author: c53763
 */
#include <cfg-util/cfg-util.h>
#include "zmq_public_data.h"
#include "ingest_recorder_param.h"
#include "executive_global_state.h"
#include "debug.h"

#define DATADISK_REFORMAT_STATE_UPDATE 1

typedef struct __attribute__((__packed__)) recorder_env_DEFINITION {
	uint8_t ares_node; //should always be 0
	uint8_t app_node; //should always be 2
	uint8_t ff; //should always be 0xFF
	uint32_t param_id;
} recorder_env_t;

typedef struct __attribute__((__packed__)) recorder_env_state_change_DEFINITION {
	uint8_t ares_node; //should always be 0
	uint8_t app_node; //should always be 2
	uint8_t state; //should always be 0
	uint8_t change; //the state change update
} recorder_env_state_change;

#define env_decoded(env_buf) ((recorder_env_t*)env_buf)
#define env_state_decoded(env_buf) ((recorder_env_state_change*)env_buf)

bool ingest_recorder_param(uint8_t * env_buf, size_t env_len,  uint8_t * pay, size_t pay_len) {	
	if (env_len == sizeof(recorder_env_state_change)){
		if ((env_state_decoded(env_buf)->state == 0) && (env_state_decoded(env_buf)->change == DATADISK_REFORMAT_STATE_UPDATE)){ //datadisk reformatted
			//renew the era uuid by calling this function
			executive_set_aircraft_id(global_state_ptr, &ucfg_global_ptr->id);
		}
		return true;
	}
	else if (env_len != sizeof(recorder_env_t)){
		DBUG_T("error, unexpected envelope length in ingest_recorder_param()");
		return false;
	}
	else {
		//printf("ingested %u\n", env_decoded(env_buf)->param_id);
		//put_param_by_id(env_decoded(env_buf)->param_id, (void *)pay,DATA_VALID);
		switch(env_decoded(env_buf)->param_id){
			case OP_STATE_DATALOGGING_ACTIVE:
				boolean_parameter_set(global_state_ptr->datalogging_active, ((*(uint32_t*)pay)==0) ? false : true);
				break;
			case OP_STATE_DATACOPYING_ACTIVE:
				printf("OP_STATE_DATACOPYING_ACTIVE %u\n", (*(uint32_t*)pay));
				boolean_parameter_set(global_state_ptr->datacopying_active, ((*(uint32_t*)pay)==0) ? false : true);
				break;
			case OP_STATE_DATA_MEDIA_ERROR:
				boolean_parameter_set(global_state_ptr->media_error, ((*(uint32_t*)pay)==0) ? false : true);
				break;
			default: break;
		}
	}
	return true;
}


