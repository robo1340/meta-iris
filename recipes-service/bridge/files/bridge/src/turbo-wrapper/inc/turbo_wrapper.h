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

#ifndef _TURBO_WRAPPER_H
#define _TURBO_WRAPPER_H
#endif 

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define TURBO_DECODE_DEFAULT_ITER 4 ///<default number of turbo decode iterations to go through
#define TURBO_DECODE_MAX_ITER	32
#define TURBO_DECODE_MIN_ITER	2

/* Maximum LTE code block size of 6144 */
#define UNCODED_BIT_LEN		TURBO_MAX_K ///<bit length of an uncoded message (6144 bits)
#define UNCODED_LEN			768         ///<byte length of an uncoded message
#define CODED_BIT_LEN       18444		///<bit length of a coded message (UNCODED_BIT_LEN * 3 + 4 * 3) bits
#define CODED_BIT_LEN_PAD   18448		///<bit length of a coded message (UNCODED_BIT_LEN * 3 + 4 * 3) bits
#define CODED_LEN			2306		///<byte length of a coded message (rounded up to the nearest byte)

typedef struct uncoded_block_t_DEF {
	uint8_t data[UNCODED_LEN];
} uncoded_block_t;

typedef struct coded_block_t_DEF {
	uint8_t data[CODED_LEN];
} coded_block_t;

bool turbo_wrapper_init(int iterations);
bool turbo_wrapper_deinit(void);

/** @brief apply turbo coding to an array of bytes, expected len is UNCODED_LEN
 *  expected output length is CODED_LEN
 */
bool turbo_wrapper_encode(uncoded_block_t * input, coded_block_t * output);

/** @brief apply turbo de-coding to an array of bytes, expected len is CODED_LEN
 *  expected output length is UNCODED_LEN
 */
bool turbo_wrapper_decode(coded_block_t * input, uncoded_block_t * output);

/**@brief convert an array of bytes to an array of bits that is 8 times the length
 * @param bytes pointer to the byte array to be converted
 * @param bytes_len the length of bytes
 * @param bits pointer to the byte array where the bit values will be stored
 * @param bits_len the length of the bits array, must be at least bytes_len*8
 * @return returns the number of bits written into the bits array on success, returns -1 on failure
*/
int bytes2bits(uint8_t * bytes, size_t bytes_len, uint8_t * bits, size_t bits_len);
int bytes2bits_ranged(uint8_t * bytes, size_t bytes_len, int8_t * bits, size_t bits_len);
int bits2bytes(uint8_t * bits, size_t bits_len, uint8_t * bytes, size_t bytes_len);




