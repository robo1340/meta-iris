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

#define TURBO_DECODE_DEFAULT_ITER 4 ///<default number of turbo decode iterations to go through
#define TURBO_DECODE_MAX_ITER	32
#define TURBO_DECODE_MIN_ITER	2

/* Maximum LTE code block size of 6144 */
//#define UNCODED_BIT_LEN		TURBO_MAX_K ///<bit length of an uncoded message (6144 bits)
//#define UNCODED_LEN			768         ///<byte length of an uncoded message
//#define CODED_BIT_LEN       18444		///<bit length of a coded message (UNCODED_BIT_LEN * 3 + 4 * 3) bits
//#define CODED_BIT_LEN_PAD   18448		///<bit length of a coded message (UNCODED_BIT_LEN * 3 + 4 * 3) bits
//#define CODED_LEN			2306		///<byte length of a coded message (rounded up to the nearest byte)

static uint8_t ubits[TURBO_MAX_K+4]; //uncoded bit buffer
static uint8_t cbits[TURBO_MAX_K*3+4*3+4]; //coded bit buffer

///<3GPP LTE turbo,
struct lte_turbo_code lte_turbo = {
	.n = 2,
	.k = 4,
	.len = TURBO_MAX_K,
	.rgen = 013,
	.gen = 015,
};

turbo_encoder_attributes_t turbo_attr = {
	.name = "3GPP LTE turbo",
	.spec = "(N=2, K=4)",
	.code = &lte_turbo, //defined in turbo_wrapper.h
	.in_len  = TURBO_MAX_K,
	.out_len = TURBO_MAX_K*3+3*4,
};

static struct tdecoder *tdec = NULL;

static bool turbo_wrapper_deinit(void){
	if (tdec != NULL){
		free(tdec);
		tdec = NULL;
	}
	return true;
}

static int turbo_wrapper_init(int iterations){
	int tdec_iterations;
	turbo_wrapper_deinit();
	if ((iterations > TURBO_DECODE_MAX_ITER) || (iterations < TURBO_DECODE_MIN_ITER)) {
		tdec_iterations = TURBO_DECODE_DEFAULT_ITER;
	}
	if (tdec != NULL) {return true;}
	tdec = alloc_tdec();
	if (tdec != NULL){
		return tdec_iterations;
	}
	return -1;
}



#define RS_ECC 4 //reed solomon error correction symbols per block
//RS_DATA+RS_ECC must be a divisor of (uncoded_bytes_len) 768
//640 usable bytes of data total
static uint8_t * tmp = NULL;

static const uint16_t VALID_BLOCK_SIZES[] = {6144,3072,1536,768,384,192,196};//,40};

#define MIN_RS_BLOCK_DATA 4
#define RS_BLOCK_ECC 4

turbo_encoder_t * turbo_encoder(uint16_t block_size_bits, int min_rs_block_size_bytes, int iterations){
	uint16_t i;
	bool ok = false;
	uint16_t j;
	
	for (i=0; i<sizeof(VALID_BLOCK_SIZES)/sizeof(VALID_BLOCK_SIZES[0]); i++){
		if (VALID_BLOCK_SIZES[i] == block_size_bits){
			ok = true;
			break;
		}
	}
	if (!ok){
		printf("ERROR: block size %u is invalid, valid block sizes listed below:\n", block_size_bits);
		for (i=0; i<sizeof(VALID_BLOCK_SIZES)/sizeof(VALID_BLOCK_SIZES[0]); i++){
			printf("\t%u bits\n", VALID_BLOCK_SIZES[i]);
		}
		return NULL;
	}
	
	if (min_rs_block_size_bytes > block_size_bits/8){
		min_rs_block_size_bytes = block_size_bits/8;
	}
	else if (min_rs_block_size_bytes < MIN_RS_BLOCK_DATA){
		min_rs_block_size_bytes = MIN_RS_BLOCK_DATA;
	}
	
	j = block_size_bits/8;
	while(true){
		if ((j % min_rs_block_size_bytes) == 0){break;}
		min_rs_block_size_bytes+=2;
	}
	
	printf("INFO turbo encoder block size set to %u bytes with %u byte reed-solomon blocks and %d iterations\n", j, min_rs_block_size_bytes, iterations);
	
	iterations = turbo_wrapper_init(iterations); //ensure the turbo wrapper is initialized
	generate_gf(); //used by the RS encoder
	
	turbo_encoder_t * to_return = (turbo_encoder_t*)malloc(sizeof(turbo_encoder_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->type = block_size_bits;
	to_return->conv_blocks = 1;
	to_return->iterations = iterations;
	turbo_attr.code->len = block_size_bits;
	turbo_attr.in_len = block_size_bits;
	turbo_attr.out_len = block_size_bits*3 + 4*3;
	memcpy(&to_return->attr, &turbo_attr, sizeof(turbo_encoder_attributes_t));
	
	to_return->uncoded_bytes_len = to_return->attr.in_len/8  + ((to_return->attr.in_len%8) ? 1 : 0);
	to_return->coded_bytes_len   = to_return->attr.out_len/8 + ((to_return->attr.out_len%8) ? 1 : 0);
	
	to_return->rs_block_len = min_rs_block_size_bytes;
	to_return->rs_blocks = to_return->uncoded_bytes_len / to_return->rs_block_len;
	if ((to_return->uncoded_bytes_len % to_return->rs_block_len) != 0){
		printf("ERROR: turbo_encoder RSS_DATA+RS_ECC does not evenly divide uncoded bytes length\n");
		return NULL;
	}
	to_return->frame_bytes_len = to_return->uncoded_bytes_len - RS_ECC*to_return->rs_blocks;
	printf("INFO: turbo encoder frame size is %u\n", to_return->frame_bytes_len);
	printf("INFO: turbo encoder coded bytes %u\n", to_return->coded_bytes_len);
	
	to_return->uncoded_bits_len_padded = to_return->attr.in_len  + ((to_return->attr.in_len%8 != 0) ? (8-to_return->attr.in_len%8):0);
	to_return->coded_bits_len_padded = to_return->attr.out_len  + ((to_return->attr.out_len%8 != 0) ? (8-to_return->attr.out_len%8):0);
	
	//printf("%d->%d : %u->%u\n", to_return->attr.in_len, to_return->attr.out_len, to_return->uncoded_bits_len_padded, to_return->coded_bits_len_padded);
	
	to_return->uncoded_bytes = malloc(to_return->uncoded_bytes_len);
	to_return->coded_bytes = malloc(to_return->coded_bytes_len);
	//if (tmp == NULL){
	turbo_interleaver_init();
	//	tmp = malloc(to_return->uncoded_bytes_len);
	//}
	if ((to_return->uncoded_bytes == NULL) || (to_return->coded_bytes == NULL)) {return NULL;}
	
	memset(to_return->uncoded_bytes, 0, to_return->uncoded_bytes_len);
	memset(to_return->coded_bytes, 0, to_return->coded_bytes_len);
	
	return to_return;
}

uint8_t * turbo_encoder_encode(turbo_encoder_t * enc, uint8_t * input, uint32_t input_len){
	//printf("INFO: turbo_encoder_encode()\n");
	uint32_t i,j, k;
	int rc;
	uint32_t actual_len = 0;
	uint8_t * rs_block = NULL;
	
	if (input_len != enc->frame_bytes_len){
		printf("ERROR: turbo_encoder_encode(%u) expected input length of %u\n", enc->type, enc->uncoded_bytes_len);
		return NULL;
	}

	//Reed solomon encoding
	k = enc->rs_block_len-RS_BLOCK_ECC;
	for (i=0,j=0; i<input_len; i+=k,j+=enc->rs_block_len){
		//printArrHex(&input[i], k);
		rs_block = encode_message(&input[i], k, 2, &actual_len); //rs encode
		memcpy(&enc->uncoded_bytes[j], rs_block, enc->rs_block_len);
		free(rs_block);
		assert(actual_len == enc->rs_block_len);
	}

	rc = bytes2bits(enc->uncoded_bytes, enc->uncoded_bytes_len, ubits, enc->uncoded_bits_len_padded); //bit-lify the input byte array and store in ubits
	if (rc != (int)enc->uncoded_bits_len_padded) {return NULL;}

	rc = lte_turbo_encode(enc->attr.code, ubits, &cbits[0], &cbits[enc->attr.in_len+4], &cbits[2*enc->attr.in_len+8]);
	if (rc != enc->attr.out_len) {
		printf("ERROR turbo_wrapper_encode() failed encoding length check %d\n", rc);
		return NULL;
	}
	rc = bits2bytes(cbits, enc->attr.out_len, enc->coded_bytes, enc->coded_bytes_len); //debit-lify the input byte array and store in output
	if (rc != (int)enc->coded_bytes_len) {return NULL;}

	return enc->coded_bytes;
}

uint8_t * turbo_encoder_decode(turbo_encoder_t * enc, uint8_t * input, uint32_t input_len){
	//printf("INFO: turbo_encoder_decode()\n");
	uint32_t i,j,k;
	int rc;
	uint32_t len;
	uint8_t * rs_block;
	if (input_len != enc->coded_bytes_len){
		printf("WARNING: encoder_decode(%u) expected input length of %u\n", enc->type, enc->coded_bytes_len);
		return NULL;
	}
	
	if (tdec == NULL) {return NULL;} 
	//memset(cbits, 0, enc->coded_bits_len_padded);
	rc = bytes2bits_ranged(input, enc->coded_bytes_len, (int8_t*)cbits, enc->coded_bits_len_padded);
	if (rc != (int)enc->coded_bits_len_padded) {return NULL;}
	
	//memset(ubits, 0, enc->attr.in_len);
	rc = lte_turbo_decode_unpack(tdec, enc->attr.in_len, enc->iterations, ubits, (int8_t*)&cbits[0], (int8_t*)&cbits[enc->attr.in_len+4], (int8_t*)&cbits[2*enc->attr.in_len+8]);
	if (rc != 0){return NULL;}

	rc = bits2bytes(ubits, enc->attr.in_len, enc->uncoded_bytes, enc->uncoded_bytes_len);
	if (rc != (int)enc->uncoded_bytes_len) {return NULL;}
	
	//Reed solomon decoding
	memset(enc->coded_bytes, 0, enc->frame_bytes_len);
	k = enc->rs_block_len-RS_BLOCK_ECC;
	for (i=0,j=0,rc=0; j<enc->uncoded_bytes_len; i+=k,j+=enc->rs_block_len,rc++){
		//printf("%d,%d,%u\n",i,j,enc->rs_block_len);
		rs_block = decode_message(&enc->uncoded_bytes[j], enc->rs_block_len, 2, &len);
		if (rs_block == NULL){
			printf("WARNING: reed solomon decoding failed on block %d\n", rc);
			return NULL;
		}
		assert(len==k); 
		memcpy(&enc->coded_bytes[i], rs_block, k);
		free(rs_block);
	}

	return enc->coded_bytes;
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
	turbo_interleaver_deinit();
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}


/*
const conv_encoder_attributes_t types[] = {
	{
		.name = "GSM xCCH",
		.spec = "(N=2, K=5, non-recursive, flushed, not punctured)",
		.code = &gsm_conv_xcch,
		.in_len  = 224, //28
		.out_len = 456, //57
	},
	{
		.name = "WiMax FCH",
		.spec = "(N=2, K=7, non-recursive, tail-biting, non-punctured)",
		.code = &wimax_conv_fch,
		.in_len  = 48, //6
		.out_len = 96, //12
	},
	{
		.name = "LTE PBCH",
		.spec = "(N=3, K=7, non-recursive, tail-biting, non-punctured)",
		.code = &lte_conv_pbch,
		.in_len  = 512,
		.out_len = 1536,
	},
};
*/

//supported types
////error_test(GSM_xCCH);
//error_test(WiMax_FCH);
////error_test(LTE_PBCH);

//make uncoded bytes length 336
#define UNCODED_BYTES 336 //supports WiMax_FCH and GSM_xCCH
//#define UNCODED_BYTES 240 //supports WiMax_FCH

#define CONV_RS_DATA	24 //reed solomon data symbols per block
//#define CONV_RS_DATA	38 //reed solomon data symbols per block
//#define CONV_RS_DATA	36 //for when UNCODED_BYTES=240
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
		printf("ERROR turbo_encoder CONV_RS_DATA+CONV_RS_ECC does not evenly divide uncoded bytes length\n");
		printf("%u %u %u\n", to_return->uncoded_bytes_len, to_return->conv_blocks, to_return->rs_block_len);
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