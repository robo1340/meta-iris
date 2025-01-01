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
#include <assert.h>

#include "turbofec/turbo.h"
#include "rs.h"
#include "turbo_encoder.h"
#include "interleaver.h"
#include "print_util.h"

///<3GPP LTE turbo,
const struct lte_turbo_code lte_turbo2 = {
	.n = 2,
	.k = 4,
	.len = UNCODED_BIT_LEN,
	.rgen = 013,
	.gen = 015,
};

turbo_encoder_attributes_t turbo_attr = {
	.name = "3GPP LTE turbo",
	.spec = "(N=2, K=4)",
	.code = &lte_turbo2, //defined in turbo_wrapper.h
	.in_len  = UNCODED_BIT_LEN,
	.out_len = CODED_BIT_LEN,
};

#define RS_DATA	20 //reed solomon data symbols per block
#define RS_ECC 4 //reed solomon error correction symbols per block
//RS_DATA+RS_ECC must be a divisor of (uncoded_bytes_len) 768
//640 usable bytes of data total
static uint8_t * tmp = NULL;

turbo_encoder_t * turbo_encoder(uint8_t type){
	turbo_wrapper_init(0); //ensure the turbo wrapper is initialized
	generate_gf(); //used by the RS encoder
	
	turbo_encoder_t * to_return = (turbo_encoder_t*)malloc(sizeof(turbo_encoder_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->type = type;
	memcpy(&to_return->attr, &turbo_attr, sizeof(turbo_encoder_attributes_t));
	
	to_return->uncoded_bytes_len = to_return->attr.in_len/8  + ((to_return->attr.in_len%8) ? 1 : 0);
	to_return->coded_bytes_len   = to_return->attr.out_len/8 + ((to_return->attr.out_len%8) ? 1 : 0);
	
	to_return->rs_block_len = RS_DATA+RS_ECC;
	to_return->rs_blocks = to_return->uncoded_bytes_len / to_return->rs_block_len;
	if ((to_return->uncoded_bytes_len % to_return->rs_block_len) != 0){
		printf("ERROR turbo_encoder RSS_DATA+RS_ECC does not evenly divide uncoded bytes length\n");
		return NULL;
	}
	to_return->frame_bytes_len = to_return->uncoded_bytes_len - RS_ECC*to_return->rs_blocks;
	
	to_return->uncoded_bits_len_padded = to_return->attr.in_len  + ((to_return->attr.in_len%8 != 0) ? (8-to_return->attr.in_len%8):0);
	to_return->coded_bits_len_padded = to_return->attr.out_len  + ((to_return->attr.out_len%8 != 0) ? (8-to_return->attr.out_len%8):0);
	
	//printf("%d->%d : %u->%u\n", to_return->attr.in_len, to_return->attr.out_len, to_return->uncoded_bits_len_padded, to_return->coded_bits_len_padded);
	
	assert(to_return->uncoded_bytes_len == sizeof(uncoded_block_t));
	assert(to_return->coded_bytes_len == sizeof(coded_block_t));
	
	to_return->uncoded_bytes = malloc(to_return->uncoded_bytes_len);
	to_return->coded_bytes = malloc(to_return->coded_bytes_len);
	if (tmp == NULL){
		turbo_interleaver_init();
		tmp = malloc(to_return->uncoded_bytes_len);
	}
	if ((to_return->uncoded_bytes == NULL) || (to_return->coded_bytes == NULL) || (tmp==NULL)) {return NULL;}
	
	memset(to_return->uncoded_bytes, 0, to_return->uncoded_bytes_len);
	memset(to_return->coded_bytes, 0, to_return->coded_bytes_len);
	
	return to_return;
}

uint8_t * turbo_encoder_encode(turbo_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	//printf("INFO: turbo_encoder_encode()\n");
	uint32_t i,j;
	int rc;
	uint32_t actual_len = 0;
	uint8_t * rs_block = NULL;
	
	if (input_len != encoder->frame_bytes_len){
		printf("WARNING: encoder_encode(%u) expected input length of %u\n", encoder->type, encoder->uncoded_bytes_len);
		return NULL;
	}

	//Reed solomon encoding
	for (i=0,j=0; i<input_len; i+=RS_DATA,j+=encoder->rs_block_len){
		//printArrHex(&input[i], RS_DATA);
		rs_block = encode_message(&input[i], RS_DATA, 2, &actual_len); //rs encode
		memcpy(&tmp[j], rs_block, encoder->rs_block_len);
		free(rs_block);
		assert(actual_len == encoder->rs_block_len);
	}
	
	//Monte Carlo simulation found that interleaving didn't make any difference
	//interleaving
	//rc = turbo_interleave(encoder->uncoded_bytes_len, tmp, encoder->uncoded_bytes);
	//assert(rc==0);
	memcpy(encoder->uncoded_bytes, tmp, encoder->uncoded_bytes_len);
	
	
	if (!turbo_wrapper_encode( (uncoded_block_t*)encoder->uncoded_bytes, (coded_block_t*)encoder->coded_bytes)){
		return NULL;
	}
	return encoder->coded_bytes;
}

uint8_t * turbo_encoder_decode(turbo_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	//printf("INFO: turbo_encoder_decode()\n");
	uint32_t i,j;
	int rc;
	uint32_t len;
	uint8_t * rs_block;
	if (input_len != encoder->coded_bytes_len){
		printf("WARNING: encoder_decode(%u) expected input length of %u\n", encoder->type, encoder->coded_bytes_len);
		return NULL;
	}
	if (!turbo_wrapper_decode((coded_block_t*)input, (uncoded_block_t*)encoder->uncoded_bytes)){
		return NULL;
	}
	
	//deinterleaving
	//rc = turbo_deinterleave(encoder->uncoded_bytes_len, encoder->uncoded_bytes, tmp);
	//assert(rc==0);
	memcpy(tmp, encoder->uncoded_bytes, encoder->uncoded_bytes_len);
	
	//Reed solomon decoding
	memset(encoder->coded_bytes, 0, encoder->frame_bytes_len);
	for (i=0,j=0,rc=0; j<encoder->uncoded_bytes_len; i+=RS_DATA,j+=encoder->rs_block_len,rc++){
		//printf("%d,%d,%u\n",i,j,encoder->rs_block_len);
		rs_block = decode_message(&tmp[j], encoder->rs_block_len, 2, &len);
		if (rs_block == NULL){
			printf("WARNING: reed solomon decoding failed on block %d\n", rc);
			return NULL;
		}
		assert(len==RS_DATA); 
		memcpy(&encoder->coded_bytes[i], rs_block, RS_DATA);
		free(rs_block);
	}

	return encoder->coded_bytes;
}

void turbo_encoder_destroy(turbo_encoder_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying turbo_encoder_t at 0x%p\n", (void*)*to_destroy);
#endif
	turbo_wrapper_deinit();
	if ((*to_destroy)->uncoded_bytes != NULL){
		free((*to_destroy)->uncoded_bytes);
	}
	if ((*to_destroy)->coded_bytes != NULL){
		free((*to_destroy)->coded_bytes);
	}
	if (tmp != NULL){
		free(tmp);
		tmp = NULL;
		turbo_interleaver_deinit();
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}


//supported types
//error_test(GSM_xCCH);
//error_test(WiMax_FCH);
////error_test(LTE_PBCH);

//make uncoded bytes length 336
#define UNCODED_BYTES 336

#define CONV_RS_DATA	24 //reed solomon data symbols per block
//#define CONV_RS_DATA	38 //reed solomon data symbols per block
#define CONV_RS_ECC 4 //reed solomon error correction symbols per block
//CONV_RS_DATA+CONV_RS_ECC must be a divisor of UNCODED_BYTES
//640 usable bytes of data total

conv_encoder_t * conv_encoder(uint8_t type){	
	generate_gf(); //used by the RS encoder
	
	conv_encoder_t * to_return = (conv_encoder_t*)malloc(sizeof(conv_encoder_t));
	if (to_return == NULL) {return NULL;}

	to_return->enc = encoder(type);
	if (to_return->enc == NULL) {return NULL;}

	to_return->uncoded_bytes_len = to_return->enc->attr.in_len/8  + ((to_return->enc->attr.in_len%8) ? 1 : 0);
	to_return->coded_bytes_len   = to_return->enc->attr.out_len/8 + ((to_return->enc->attr.out_len%8) ? 1 : 0);

	if ((type == GSM_xCCH) || (type == WiMax_FCH)){
		to_return->conv_blocks = UNCODED_BYTES/to_return->uncoded_bytes_len;
	}
	else {
		printf("conv_encoder(%u) invalid type\n", type);
		return NULL;
	}
	
	to_return->rs_block_len = CONV_RS_DATA+CONV_RS_ECC;
	to_return->rs_blocks = (to_return->uncoded_bytes_len*to_return->conv_blocks) / to_return->rs_block_len;
	//to_return->rs_blocks = (to_return->uncoded_bytes_len) / to_return->rs_block_len;
	if ((to_return->uncoded_bytes_len*to_return->conv_blocks % to_return->rs_block_len) != 0){
		printf("ERROR turbo_encoder RSS_DATA+CONV_RS_ECC does not evenly divide uncoded bytes length\n");
		return NULL;
	}
	to_return->frame_bytes_len = (to_return->uncoded_bytes_len*to_return->conv_blocks) - CONV_RS_ECC*to_return->rs_blocks;
	
	to_return->uncoded_bits_len_padded = to_return->enc->attr.in_len  + ((to_return->enc->attr.in_len%8 != 0) ? (8-to_return->enc->attr.in_len%8):0);
	to_return->coded_bits_len_padded = to_return->enc->attr.out_len  + ((to_return->enc->attr.out_len%8 != 0) ? (8-to_return->enc->attr.out_len%8):0);
	
	//printf("%d->%d : %u->%u\n", to_return->enc->attr.in_len, to_return->enc->attr.out_len, to_return->uncoded_bits_len_padded, to_return->coded_bits_len_padded);
	
	//assert(to_return->uncoded_bytes_len == sizeof(uncoded_block_t));
	//assert(to_return->coded_bytes_len == sizeof(coded_block_t));
	
	to_return->uncoded_bytes = malloc(to_return->uncoded_bytes_len * to_return->conv_blocks);
	to_return->coded_bytes = malloc(to_return->coded_bytes_len * to_return->conv_blocks);
	if (tmp == NULL){
		turbo_interleaver_init();
		tmp = malloc(to_return->uncoded_bytes_len * to_return->conv_blocks);
	}
	if ((to_return->uncoded_bytes == NULL) || (to_return->coded_bytes == NULL) || (tmp==NULL)) {return NULL;}
	
	memset(to_return->uncoded_bytes, 0, to_return->uncoded_bytes_len * to_return->conv_blocks);
	memset(to_return->coded_bytes, 0, to_return->coded_bytes_len * to_return->conv_blocks);
	
	return to_return;
}

uint8_t * conv_encoder_encode(conv_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	//printf("INFO: conv_encoder_encode()\n");
	uint32_t i,j;
	int rc;
	uint32_t actual_len = 0;
	uint8_t * rs_block = NULL;
	uint8_t * coded_bytes = NULL;
	
	if (input_len != encoder->frame_bytes_len){
		printf("WARNING: conv_encoder_encode(%u) expected input length of %u\n", encoder->enc->type, encoder->uncoded_bytes_len);
		return NULL;
	}

	//Reed solomon encoding
	for (i=0,j=0; i<input_len; i+=CONV_RS_DATA,j+=encoder->rs_block_len){
		//printArrHex(&input[i], CONV_RS_DATA);
		rs_block = encode_message(&input[i], CONV_RS_DATA, 2, &actual_len); //rs encode
		memcpy(&tmp[j], rs_block, encoder->rs_block_len);
		free(rs_block);
		assert(actual_len == encoder->rs_block_len);
	}
	
	//interleaving
	rc = turbo_interleave(encoder->uncoded_bytes_len*encoder->conv_blocks, tmp, encoder->uncoded_bytes);
	assert(rc==0);
	//memcpy(encoder->uncoded_bytes, tmp, encoder->uncoded_bytes_len*encoder->conv_blocks);
	
	for (i=0; i<encoder->conv_blocks; i++){
		coded_bytes = encoder_encode(encoder->enc, encoder->uncoded_bytes+(i*encoder->uncoded_bytes_len), encoder->uncoded_bytes_len);
		if (coded_bytes == NULL){
			return NULL;
		}
		memcpy(encoder->coded_bytes+(i*encoder->coded_bytes_len), coded_bytes, encoder->coded_bytes_len);
	}
	return encoder->coded_bytes;
}

uint8_t * conv_encoder_decode(conv_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	//printf("INFO: conv_encoder_decode()\n");
	uint32_t i,j;
	int rc;
	uint32_t len;
	uint8_t * rs_block;
	uint8_t * decoded_bytes = NULL;
	
	if (input_len != encoder->coded_bytes_len*encoder->conv_blocks){
		printf("WARNING: conv_encoder_decode(%u) expected input length of %u\n", encoder->enc->type, encoder->coded_bytes_len*encoder->conv_blocks);
		return NULL;
	}
	
	for (i=0; i<encoder->conv_blocks; i++){
		decoded_bytes = encoder_decode(encoder->enc, input+(i*encoder->coded_bytes_len), encoder->coded_bytes_len);
		if (decoded_bytes == NULL){
			return NULL;
		}
		memcpy(encoder->uncoded_bytes+(i*encoder->uncoded_bytes_len), decoded_bytes, encoder->uncoded_bytes_len);
	}
	
	//deinterleaving
	rc = turbo_deinterleave(encoder->uncoded_bytes_len*encoder->conv_blocks, encoder->uncoded_bytes, tmp);
	assert(rc==0);
	//memcpy(tmp, encoder->uncoded_bytes, encoder->uncoded_bytes_len*encoder->conv_blocks);
	
	//Reed solomon decoding
	memset(encoder->coded_bytes, 0, encoder->frame_bytes_len);
	for (i=0,j=0,rc=0; j<encoder->uncoded_bytes_len*encoder->conv_blocks; i+=CONV_RS_DATA,j+=encoder->rs_block_len,rc++){
		//printf("%d,%d,%u\n",i,j,encoder->rs_block_len);
		rs_block = decode_message(&tmp[j], encoder->rs_block_len, 2, &len);
		if (rs_block == NULL){
			printf("WARNING: reed solomon decoding failed on block %d\n", rc);
			return NULL;
		}
		assert(len==CONV_RS_DATA); 
		memcpy(&encoder->coded_bytes[i], rs_block, CONV_RS_DATA);
		free(rs_block);
	}

	return encoder->coded_bytes;
}

void conv_encoder_destroy(conv_encoder_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying conv_encoder_t at 0x%p\n", (void*)*to_destroy);
#endif
	encoder_t * enc = (*to_destroy)->enc;
	encoder_destroy(&enc);
	if ((*to_destroy)->uncoded_bytes != NULL){
		free((*to_destroy)->uncoded_bytes);
	}
	if ((*to_destroy)->coded_bytes != NULL){
		free((*to_destroy)->coded_bytes);
	}
	if (tmp != NULL){
		//free(tmp);
		//tmp = NULL;
		//turbo_interleaver_deinit();
	}
	free(*to_destroy);
	*to_destroy = NULL;
	
	return;	
}