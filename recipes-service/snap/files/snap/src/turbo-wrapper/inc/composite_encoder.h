#pragma once


#ifndef _COMPOSITE_ENCODER_H
#define _COMPOSITE_ENCODER_H
#endif 

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "turbo_wrapper.h"

#include <turbofec/conv.h>
#include "conv_wrapper.h"

typedef struct composite_encoder_DEFINITION composite_encoder_t;

//Data Encoder Object
//
//encoding process
//1. start with n uncoded bytes, n must be divisible by the reed solo block length
//2. reed solo encode each block of n and append the 4 ECC bytes to the end of each block
//3. the reed solo encoded payload is now of length j and must be divisible by the convolutional encoder input length (6 bytes)
//4. for each convolutional block, apply the WiMax FCH encoder to turn the 6 byte block into an encoded 12 byte block


typedef struct __attribute__((__packed__)) rs_settings_DEFINITION {
	uint16_t uncoded_len;///<the uncoded length (in bytes) of each reed solo block
	uint8_t ecc_mode; ///<0 (disabled), 1, or 2
	uint16_t ecc_len; 	 ///< the number of ecc bytes in the reed solo block, set to 0 to disable
	uint16_t coded_len;
	uint32_t err_cnt; ///<running total of errors that have been caught
	uint32_t corrected_err_cnt;
} rs_settings_t;

typedef struct __attribute__((__packed__)) conv_settings_DEFINITION {
	uint16_t uncoded_len; 	///<uncoded length (bytes)
	uint16_t coded_len; 	///<coded length (bytes)
	encoder_t * enc; ///<the WiMaX FCH encoder object
	bool disabled;
} conv_settings_t;

struct __attribute__((__packed__)) composite_encoder_DEFINITION {
	uint16_t conv_blocks; ///<the number of convolutional code blocks used
	uint16_t rs_blocks; ///<the number of reed solomon blocks used
	rs_settings_t rs;
	uint16_t uncoded_len; ///<length (in bytes) of an uncoded payload
	uint16_t rs_coded_len; ///<length (in bytes) of a payload that is rs encoded but not convolutionally encoded
	uint16_t coded_len; ///<length (in bytes) of a payload that is rs encoded and convolutionally encoded
	conv_settings_t conv;
	uint8_t * uncoded_bytes;
	uint8_t * coded_bytes;
};

composite_encoder_t * composite_encoder(uint32_t uncoded_bytes_len, uint8_t rs_encoder_mode, uint8_t rs_block_len, bool disable_convolutional, uint32_t * coded_bytes_len);

uint8_t * composite_encoder_encode(composite_encoder_t * encoder, uint8_t * input, uint32_t input_len);

uint8_t * composite_encoder_decode(composite_encoder_t * encoder, uint8_t * input, uint32_t input_len);

void composite_encoder_report(composite_encoder_t * encoder);

void composite_encoder_destroy(composite_encoder_t ** to_destroy);



