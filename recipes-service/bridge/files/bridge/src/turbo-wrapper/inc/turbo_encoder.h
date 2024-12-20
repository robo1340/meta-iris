#pragma once


/*
 *
 * This file is part of pyA20.
 * spi_lib.c is python SPI extension.
 *
 * Copyright (c) 2014 Stefan Mavrodiev @ OLIMEX LTD, <support@olimex.com>
 *
 * pyA20 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _TURBO_ENCODER_H
#define _TURBO_ENCODER_H
#endif 

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "turbo_wrapper.h"

#include <turbofec/conv.h>


typedef struct turbo_encoder_DEFINITION turbo_encoder_t; 					///<definition for a sub-payload making up a part of the payload of a frame

typedef struct turbo_encoder_attributes_DEF {
	const char name[24];
	const char spec[128];
	const struct lte_turbo_code *code;
	int in_len; //uncoded length in bits
	int out_len; //coded length in bits
} turbo_encoder_attributes_t;

struct __attribute__((__packed__)) turbo_encoder_DEFINITION {
	uint8_t type;
	turbo_encoder_attributes_t attr;
	uint16_t rs_blocks; //the number of reed solomon blocks in the encoder
	uint16_t rs_block_len;
	uint32_t frame_bytes_len; //length of the dataframe before RS encoding and turbo encoding
	uint8_t * uncoded_bytes;
	uint32_t uncoded_bytes_len; //length of the dataframe after RS encoding but before turbo encoding
	uint32_t uncoded_bits_len_padded;
	uint8_t * coded_bytes;
	uint32_t coded_bytes_len;
	uint32_t coded_bits_len_padded;
};

turbo_encoder_t * turbo_encoder(uint8_t type);

/** @brief encoder encode
 *  @param encoder the encoder object
 *  @param input
 *  @param input_len must be the same value as encoder->uncoded_bytes_len
 */
uint8_t * turbo_encoder_encode(turbo_encoder_t * encoder, uint8_t * input, uint32_t input_len);

/** @brief encoder decode
 *  @param encoder the encoder object
 *  @param input
 *  @param input_len must be the same value as encoder->coded_bytes_len
 */
uint8_t * turbo_encoder_decode(turbo_encoder_t * encoder, uint8_t * input, uint32_t input_len);

void turbo_encoder_destroy(turbo_encoder_t ** to_destroy);




