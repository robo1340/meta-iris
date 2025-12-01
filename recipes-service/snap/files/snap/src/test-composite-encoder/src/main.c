//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h> 

#include "composite_encoder.h"
#include "print_util.h"

//int r = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

/* Bit error rate test */
static int error_test(uint8_t uncoded_len, uint8_t rs_mode, uint8_t rs_block){
	int i, ober = 0;
	
	composite_encoder_t * enc = composite_encoder(uncoded_len, rs_mode, rs_block, NULL);
	if (enc == NULL){return -1;}
	
	assert(enc != NULL);
	
	uint8_t input[640];
	//uint8_t * input = malloc(enc->frame_bytes_len);
	//printf("uncoded_len=%u\n", enc->uncoded_len);
	for (i=0; i<enc->uncoded_len; i++){
		input[i] = i%255;
	}
	printArrHex(input, enc->uncoded_len);

	uint8_t * coded = composite_encoder_encode(enc, input, enc->uncoded_len);
	assert(coded != NULL);

	//printArrHex(input, enc->frame_bytes_len);
	uint8_t channel[640];
	memcpy(channel, coded, enc->coded_len);
	//random
	/*
	int r;
	for (i=0; i<enc->coded_len; i++){
		r = rand() % 100;
		if (r > 98){ //bursty errors, the reed solomon code will deal with these well
			coded[i] = ~coded[i];
		}
		if (r > 75) { //single bit errors, the convolutional code will deal with these well
			coded[i] ^= 1 << (rand()%8);
		}
		
	}
	*/
	
	uint8_t * decoded = composite_encoder_decode(enc, coded, enc->coded_len);
	if (decoded == NULL){
		printf("Reed Solomon Decoding Failed\n");
		return -1;
	}
	assert(decoded != NULL);
	for (i=0, ober=0; i<enc->uncoded_len; i++) {
		if (decoded[i] != input[i]) {
			printf("%d\n",i);
			ober++;
		}
	}
	printf("FER....%d\n", ober);
	
	printArrHex(decoded, enc->uncoded_len);
	int rc = memcmp(decoded, input, enc->uncoded_len);
	if (rc == 0){
		printf("SUCCESS on type %u,%u,%u\n", uncoded_len, rs_mode, rs_block);
	} else {
		printf("FAILURE on type %u,%u,%u\n", uncoded_len, rs_mode, rs_block);
	}
	
	composite_encoder_report(enc);
	composite_encoder_destroy(&enc);
	return rc;
}

int main(void) {

	srand(time(NULL));   // Initialization, should only be called once.
	//srand(2);

	int i,rc;
	float iters = 1;
	float s,f;
	float s2,f2;
	
	//for (i=0; i<(int)iters; i++){
	//	rc = error_test(GSM_xCCH);
	//	if (rc == 0){s++;}
	//	else {f++;}
	//}
	
	uint16_t uncoded_lens[] = {100};
	uint8_t rs_modes[] = {2};
	uint16_t rs_blocks[] = {20};
	
	//uint16_t uncoded_lens[] = {100, 200, 300, 500};
	//uint8_t rs_modes[] = {2,2,2,2};
	//uint16_t rs_blocks[] = {20, 25, 30, 40, 50};
	int opt1,opt2,opt3;
	
	for (opt1=0; opt1<sizeof(uncoded_lens)/sizeof(uint16_t); opt1++){
		for (opt2=0; opt2<sizeof(rs_modes)/sizeof(uint8_t); opt2++){
			for (opt3=0; opt3<sizeof(rs_blocks)/sizeof(uint16_t); opt3++){
	
	
	for (i=0; i<(int)iters; i++){
		rc = error_test(uncoded_lens[opt1],rs_modes[opt2],rs_blocks[opt3]);
		if (rc == 0){s2++;}
		else {f2++;}
	}
	
	}}}
	
	//printf("GSM_xCCH:  %2.0f successes, %2.0f failures, %3.1f%%\n", s, f, (s/iters)*100);
	printf("WiMax_FCH: %2.0f successes, %2.0f failures, %3.1f%%\n", s2, f2, (s2/iters)*100);
}
