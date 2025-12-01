/*
 *
 * This file is based on  of pyA20.
 * spi_lib.c python SPI extension.
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
 * Adpated by Philippe Van Hecke <lemouchon@gmail.com>
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "turbofec/turbo.h"
#include "turbo_wrapper.h"

/*
///<3GPP LTE turbo,
const struct lte_turbo_code lte_turbo = {
	.n = 2,
	.k = 4,
	.len = UNCODED_BIT_LEN,
	.rgen = 013,
	.gen = 015,
};
*/

int bytes2bits(uint8_t * bytes, size_t bytes_len, uint8_t * bits, size_t bits_len){
	if (bits_len < (bytes_len*8)) {return -1;}
	uint32_t i;
	int x;
	for (i=0; i<bytes_len*8; i++) {
		x = 7 - (i%8); //get the position of the bit in the byte
		bits[i] = ((1 << x ) & (bytes[i/8])) >> x;
		//bits[i] = ((1 << (i % 8)) & (bytes[i/8])) >> (i % 8);
	}
	return i;
}

#define BYTES2BITS_RANGED_HIGH 127
#define BYTES2BITS_RANGED_LOW -128

int bytes2bits_ranged(uint8_t * bytes, size_t bytes_len, int8_t * bits, size_t bits_len){
	if (bits_len < (bytes_len*8)) {return -1;}
	uint32_t i;
	int x;
	for (i=0; i<bytes_len*8; i++) {
		x = 7 - (i%8); //get the position of the bit in the byte
		bits[i] = (((1 << x ) & (bytes[i/8])) >> x) ? BYTES2BITS_RANGED_HIGH : BYTES2BITS_RANGED_LOW;
		//bits[i] = (((1 << (i % 8)) & (bytes[i/8])) >> (i % 8)) ? BYTES2BITS_RANGED_HIGH : BYTES2BITS_RANGED_LOW;
	}
	return i;
}

int bits2bytes(uint8_t * bits, size_t bits_len, uint8_t * bytes, size_t bytes_len){
	uint32_t to_return = (bits_len/8) + ((bits_len%8)!=0); //calculate the number of bytes that will be filled
	if (bytes_len < to_return) {return -1;}
	uint32_t i;
	memset(bytes, 0, bytes_len);
	for (i=0; i<bits_len; i++) {
		//bytes[i/8] |= bits[i] << (i%8);
		bytes[i/8] |= bits[i] << (7-(i%8));
	}
	return (int)to_return;
}

/*
bool turbo_wrapper_encode(uncoded_block_t * input, coded_block_t * output){
	int rc;
	rc = bytes2bits(input->data, sizeof(input->data), ubits, UNCODED_BIT_LEN); //bit-lify the input byte array and store in ubits
	if (rc != UNCODED_BIT_LEN) {return false;}

	rc = lte_turbo_encode(&lte_turbo, ubits, &cbits[0], &cbits[UNCODED_BIT_LEN+4], &cbits[UNCODED_BIT_LEN+UNCODED_BIT_LEN+8]);
	if (rc != CODED_BIT_LEN) {
		printf("ERROR turbo_wrapper_encode() failed encoding length check %d\n", rc);
		return false;
	}

	rc = bits2bytes(cbits, CODED_BIT_LEN, output->data, CODED_LEN); //debit-lify the input byte array and store in output
	if (rc != CODED_LEN) {return false;}
	return true;
}
*/

/*
bool turbo_wrapper_decode(coded_block_t * input, uncoded_block_t * output){
	if (tdec == NULL) {return false;}
	int rc;
	
	rc = bytes2bits_ranged(input->data, CODED_LEN, (int8_t*)cbits, CODED_BIT_LEN_PAD);
	if (rc != CODED_BIT_LEN_PAD) {return -1;}
	
	lte_turbo_decode_unpack(tdec, UNCODED_BIT_LEN, tdec_iterations, ubits, (int8_t*)&cbits[0], (int8_t*)&cbits[UNCODED_BIT_LEN+4], (int8_t*)&cbits[UNCODED_BIT_LEN+UNCODED_BIT_LEN+8]);

	rc = bits2bytes(ubits, UNCODED_BIT_LEN, output->data, UNCODED_LEN);
	if (rc != UNCODED_LEN) {return -1;}

	return true;
}
*/