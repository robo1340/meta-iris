/*
 * ingest_recorder_param.h
 *
 *  Created on: Mar 3, 2023
 *      Author: c53763
 */

#ifndef ZMQ_SHARED_INC_INGEST_RECORDER_PARAM_H_
#define ZMQ_SHARED_INC_INGEST_RECORDER_PARAM_H_

#include <stdint.h>
#include <stdbool.h>

bool ingest_recorder_param(uint8_t * env_buf, size_t env_len,  uint8_t * pay, size_t pay_len);

#endif /* ZMQ_SHARED_INC_INGEST_RECORDER_PARAM_H_ */
