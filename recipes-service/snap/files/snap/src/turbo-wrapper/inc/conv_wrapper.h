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

#ifndef _CONV_WRAPPER_H
#define _CONV_WRAPPER_H
#endif 

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "turbo_wrapper.h"

#include <turbofec/conv.h>

//coding types indices
#define GSM_xCCH 0
#define	GPRS_CS2 1
#define GPRS_CS3 2
#define GSM_RACH 3
#define GSM_SCH  4
#define GSM_TCH_FR 5
#define GSM_TCH_AFS_12_2 6
#define GSM_TCH_AFS_10_2 7
#define GSM_TCH_AFS_7_95 8
#define GSM_TCH_AFS_7_4  9
#define GSM_TCH_AFS_6_7  10
#define GSM_TCH_AFS_5_9  11
#define GSM_TCH_AHS_7_95 12
#define GSM_TCH_AHS_7_4  13
#define GSM_TCH_AHS_6_7  14
#define GSM_TCH_AHS_5_9  15
#define GSM_TCH_AHS_5_15 16
#define GSM_TCH_AHS_4_75 17
#define WiMax_FCH 18
#define LTE_PBCH  19

typedef struct encoder_DEFINITION encoder_t; 					///<definition for a sub-payload making up a part of the payload of a frame

typedef struct conv_encoder_attributes_DEF {
	const char name[24];
	const char spec[128];
	const struct lte_conv_code *code;
	int in_len;
	int out_len;
} conv_encoder_attributes_t;

struct __attribute__((__packed__)) encoder_DEFINITION {
	uint8_t type;
	conv_encoder_attributes_t attr;
	uint8_t * uncoded_bytes;
	uint32_t uncoded_bytes_len;
	uint32_t uncoded_bits_len_padded;
	uint8_t * coded_bytes;
	uint32_t coded_bytes_len;
	uint32_t coded_bits_len_padded;
};

encoder_t * encoder(uint8_t type);

int encoder_bits_in_padded(encoder_t * encoder);
int encoder_bits_out_padded(encoder_t * encoder);

/** @brief encoder encode
 *  @param encoder the encoder object
 *  @param input
 *  @param input_len must be the same value as encoder->uncoded_bytes_len
 */
uint8_t * encoder_encode(encoder_t * encoder, uint8_t * input, uint32_t input_len);

/** @brief encoder decode
 *  @param encoder the encoder object
 *  @param input
 *  @param input_len must be the same value as encoder->coded_bytes_len
 */
uint8_t * encoder_decode(encoder_t * encoder, uint8_t * input, uint32_t input_len);

void encoder_destroy(encoder_t ** to_destroy);




