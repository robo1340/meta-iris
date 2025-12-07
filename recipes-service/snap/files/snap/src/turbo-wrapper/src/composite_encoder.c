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
#include "composite_encoder.h"
#include "print_util.h"

/*
const conv_encoder_attributes_t types[] = {
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


static bool validate_composite_encoder_inputs(uint32_t uncoded_bytes_len_desired, uint8_t rs_encoder_mode_desired, uint8_t rs_block_len_desired, 
											  uint32_t * uncoded_bytes_len_actual, uint8_t * rs_block_len){

	printf("DEBUG: validate_composite_encoder_inputs(%u,%u,%u)->", uncoded_bytes_len_desired, rs_encoder_mode_desired, rs_block_len_desired);
	uint16_t rs_coded_len;
	uint16_t rs_blocks;
	bool rs_block_divides = false;
	bool conv_block_divides = false;
	uint32_t original = uncoded_bytes_len_desired;
	uint8_t original_block_size = rs_block_len_desired;
	
	while (true){
		rs_block_divides = ((uncoded_bytes_len_desired%rs_block_len_desired) == 0);
		
		rs_blocks = uncoded_bytes_len_desired/rs_block_len_desired;
		rs_coded_len = rs_blocks*(rs_block_len_desired + rs_encoder_mode_desired*2);
		
		conv_block_divides = (rs_coded_len % 6)==0;
		
		if (!rs_block_divides){
			rs_block_len_desired++;
		}
		else if (!conv_block_divides){
			uncoded_bytes_len_desired += 2;
			rs_block_len_desired = original_block_size;
			rs_block_divides = false;
		}
		
		if (rs_block_divides && conv_block_divides){
			break;
		}
		else if (uncoded_bytes_len_desired > (2*original)){
			return false;
		}
		
	}
	
	*uncoded_bytes_len_actual=uncoded_bytes_len_desired;
	*rs_block_len = rs_block_len_desired;
	printf("FINAL: (%u,%u,%u)\n", *uncoded_bytes_len_actual, rs_encoder_mode_desired, *rs_block_len);
	return true;
}


/**@brief create an instance of a composite encoder
 * @param uncoded_bytes_len total length of uncoded input payloads
 * @param rs_encoder_mode set to 0 to disable reed solomon encoded, 1 for 2 ECC symbols, 2 for 4 ECC symbols, all other inputs are invalid
 * @param coded_bytes_len - output - point to a variable containing the length of an encoded packet, in bytes
 * @return returns a pointer to the composite_encoder_t on success, returns NULL otherwise
*/
composite_encoder_t * composite_encoder(uint32_t uncoded_bytes_len, uint8_t rs_encoder_mode, uint8_t rs_block_len, bool disable_convolutional, uint32_t * coded_bytes_len){
	printf("INFO: composite_encoder(%u, %u, %u)\n", uncoded_bytes_len, rs_encoder_mode, rs_block_len);
	generate_gf(); //generate gallois fields used by the reed solomon encoder

	if (rs_block_len > uncoded_bytes_len){
		printf("ERROR: rs_block_len %u may not be larger than uncoded bytes len %u\n", rs_block_len, uncoded_bytes_len);
		return NULL;
	}
	
	uint32_t x;
	uint8_t y;
	
	if (!validate_composite_encoder_inputs(uncoded_bytes_len, rs_encoder_mode, rs_block_len, &x, &y)){
		printf("ERROR: failed to validate inputs\n");
		return NULL;
	}
	
	uncoded_bytes_len = x;
	rs_block_len = y;
	
	assert((uncoded_bytes_len%rs_block_len) == 0);
	
	composite_encoder_t * to_return = (composite_encoder_t*)malloc(sizeof(composite_encoder_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->conv.disabled = disable_convolutional;
	to_return->conv.enc = encoder(WiMax_FCH);
	if (to_return->conv.enc == NULL) {return NULL;}
	
	to_return->conv.uncoded_len = to_return->conv.enc->attr.in_len/8  + ((to_return->conv.enc->attr.in_len%8) ? 1 : 0);
	if (!disable_convolutional){
		to_return->conv.coded_len   = to_return->conv.enc->attr.out_len/8 + ((to_return->conv.enc->attr.out_len%8) ? 1 : 0);
	} else {
		to_return->conv.coded_len = to_return->conv.uncoded_len;
	}
	
	to_return->rs.uncoded_len = rs_block_len;
	if ((rs_encoder_mode != 0) && (rs_encoder_mode != 2)){
		printf("WARNING: invalid rs_encoder_mode for composite_encoder (%u), should be 0 (disabled) or 2(enabled) defaulting to enabled\r\n", rs_encoder_mode);
		rs_encoder_mode = 2;
		return NULL; 
	}
	to_return->rs.ecc_mode = rs_encoder_mode;
	to_return->rs.ecc_len = to_return->rs.ecc_mode*2;
	to_return->rs.coded_len = to_return->rs.uncoded_len + to_return->rs.ecc_len;
	to_return->rs.err_cnt = 0;
	to_return->rs.corrected_err_cnt = 0;
	
	to_return->uncoded_len = uncoded_bytes_len;
	to_return->rs_blocks   = to_return->uncoded_len/to_return->rs.uncoded_len +  ((to_return->uncoded_len%to_return->rs.uncoded_len) ? 1 : 0);
	to_return->rs_coded_len= to_return->rs_blocks*to_return->rs.coded_len;
	
	to_return->conv_blocks = to_return->rs_coded_len/to_return->conv.uncoded_len + ((to_return->rs_coded_len%to_return->conv.uncoded_len) ? 1 : 0);
	
	assert((to_return->rs_coded_len%to_return->conv.uncoded_len) == 0);
	
	to_return->coded_len = to_return->conv_blocks * to_return->conv.coded_len;

	to_return->uncoded_bytes = malloc(to_return->rs_coded_len);
	to_return->coded_bytes = malloc(to_return->coded_len);
	if ((to_return->uncoded_bytes == NULL) || (to_return->coded_bytes == NULL)) {return NULL;}
	
	memset(to_return->uncoded_bytes, 0, to_return->rs_coded_len);
	memset(to_return->coded_bytes, 0, to_return->coded_len);
	
	if (coded_bytes_len != NULL){
		*coded_bytes_len = to_return->coded_len;
	}
	return to_return;
}

uint8_t * composite_encoder_encode(composite_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	uint32_t i,j,k;
	//int rc;
	uint32_t actual_len = 0;
	uint8_t * rs_block = NULL;
	uint8_t * coded_bytes = NULL;
	
	if (input_len != encoder->uncoded_len){
		printf("WARNING: hdr_conv_encoder_encode(%u) expected input length of %u\n", encoder->conv.enc->type, encoder->uncoded_len);
		return NULL;
	}

	//Reed solomon encoding
	for (i=0,j=0,k=0; k<encoder->rs_blocks; i+=encoder->rs.uncoded_len,j+=encoder->rs.coded_len,k++){
		//printArrHex(&input[i], HDR_CONV_RS_DATA);
		if (encoder->rs.ecc_mode != 0){
			rs_block = encode_message(&input[i], encoder->rs.uncoded_len, encoder->rs.ecc_mode, &actual_len); //rs encode, eccc mode is 1 or 2
			memcpy(&encoder->uncoded_bytes[j], rs_block, encoder->rs.coded_len);
			free(rs_block);
			assert(actual_len == encoder->rs.coded_len);
		} else { //reed solo encoding disabled
			memcpy(&encoder->uncoded_bytes[j], &input[i], encoder->rs.coded_len);
		}
	}
	
	for (i=0; i<encoder->conv_blocks; i++){
		if (!encoder->conv.disabled){
			coded_bytes = encoder_encode(encoder->conv.enc, encoder->uncoded_bytes+(i*encoder->conv.uncoded_len), encoder->conv.uncoded_len);
			if (coded_bytes == NULL){
				return NULL;
			}
			memcpy(encoder->coded_bytes+(i*encoder->conv.coded_len), coded_bytes, encoder->conv.coded_len);
		} else { //convolutional encoding disabled
			memcpy(encoder->coded_bytes+(i*encoder->conv.uncoded_len), encoder->uncoded_bytes+(i*encoder->conv.uncoded_len), encoder->conv.uncoded_len);
		}

	}
	return encoder->coded_bytes;
}

uint8_t * composite_encoder_decode(composite_encoder_t * encoder, uint8_t * input, uint32_t input_len){
	uint32_t i,j,err;
	int rc;
	uint32_t len;
	uint8_t * rs_block;
	uint8_t * decoded_bytes = NULL;
	
	if (input_len != encoder->coded_len){
		printf("WARNING: composite encoder expected input length of %u\n", encoder->coded_len);
		return NULL;
	}
	
	for (i=0; i<encoder->conv_blocks; i++){
		if (!encoder->conv.disabled){
			decoded_bytes = encoder_decode(encoder->conv.enc, input+(i*encoder->conv.coded_len), encoder->conv.coded_len);
			if (decoded_bytes == NULL){
				return NULL;
			}
			memcpy(encoder->uncoded_bytes+(i*encoder->conv.uncoded_len), decoded_bytes, encoder->conv.uncoded_len);
		} else { //convolutional encoding disabled
			memcpy(encoder->uncoded_bytes+(i*encoder->conv.uncoded_len), input+(i*encoder->conv.uncoded_len), encoder->conv.uncoded_len);
		}
	}
	
	//Reed solomon decoding
	memset(encoder->coded_bytes, 0, encoder->coded_len);
	for (i=0,j=0,rc=0; j<encoder->uncoded_len; i+=encoder->rs.uncoded_len,j+=encoder->rs.coded_len,rc++){
		//printf("%d,%d,%u\n",i,j,encoder->rs.coded_len);
		err = global_rs_error_cnt;
		if (encoder->rs.ecc_mode != 0){
			rs_block = decode_message(&encoder->uncoded_bytes[j], encoder->rs.coded_len, encoder->rs.ecc_mode, &len); //ecc mode is 1 or 2
			if (rs_block == NULL){
				printf("WARNING: reed solomon decoding failed on block %d\n", rc);
				encoder->rs.err_cnt++;
				return NULL;
			} else {
				if (global_rs_error_cnt != err){ //errors were corrected
					encoder->rs.corrected_err_cnt += (global_rs_error_cnt - err);
				}
			}
			assert(len==encoder->rs.uncoded_len); 
			memcpy(&encoder->coded_bytes[i], rs_block, encoder->rs.uncoded_len);
			free(rs_block);
		} else {
			memcpy(&encoder->coded_bytes[i], &encoder->uncoded_bytes[j], encoder->rs.uncoded_len);
		}
	}

	return encoder->coded_bytes;
}

void composite_encoder_report(composite_encoder_t * encoder){
	printf("composite_encoder | %u->%u->%u\n", encoder->uncoded_len, encoder->rs_coded_len, encoder->coded_len);
	printf("  RS Settings,    | mode=%u, %u->%u\n", encoder->rs.ecc_mode, encoder->rs.uncoded_len, encoder->rs.coded_len);
	printf("  RS ERRORS       | failed=%u, corrected=%u\n", encoder->rs.err_cnt, encoder->rs.corrected_err_cnt);
	printf("  CONV Settings,  | %u->%u\n", encoder->conv.uncoded_len, encoder->conv.coded_len);
	printf("  RS Blocks %u, Conv Blocks %u\n", encoder->rs_blocks, encoder->conv_blocks);
	
}

void composite_encoder_destroy(composite_encoder_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying composite_encoder_t at 0x%p\n", (void*)*to_destroy);
#endif
	encoder_t * enc = (*to_destroy)->conv.enc;
	encoder_destroy(&enc);
	if ((*to_destroy)->uncoded_bytes != NULL){
		free((*to_destroy)->uncoded_bytes);
	}
	if ((*to_destroy)->coded_bytes != NULL){
		free((*to_destroy)->coded_bytes);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	
	return;	
}