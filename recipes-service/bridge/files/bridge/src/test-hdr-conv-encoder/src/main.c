//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "turbo_encoder.h"
#include "print_util.h"

//int r = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

/* Bit error rate test */
static int error_test(uint8_t type){
	int i, ober = 0;
	printf("\nerror_test(%u,", type);
	
	conv_encoder_t * enc = hdr_conv_encoder();
	
	assert(enc != NULL);
	printf("\"%s\")\n", enc->enc->attr.name);
	printf("\"%s\" %d->%d\n", enc->enc->attr.spec, enc->enc->attr.in_len, enc->enc->attr.out_len);
	printf("frame len: %u\n", enc->frame_bytes_len);
	printf("%u -> %u bytes\n", enc->uncoded_bytes_len, enc->coded_bytes_len);
	printf("%u convolutional blocks\n", enc->conv_blocks);
	
	uint8_t input[640];
	//uint8_t * input = malloc(enc->frame_bytes_len);
	printf("frame_bytes_len=%u\n", enc->frame_bytes_len);
	for (i=0; i<enc->frame_bytes_len; i++){
		input[i] = i%255;
	}
	//printArrHex(input, enc->frame_bytes_len);

	uint8_t * coded = hdr_conv_encoder_encode(enc, input, enc->frame_bytes_len);
	assert(coded != NULL);

	//printArrHex(input, enc->frame_bytes_len);

	/*
	//deterministic
	//send over channel, simulate errors
	for (i=0; i<enc->coded_bytes_len; i++){
		if ((i%2) == 0){
			//coded[i] = ~coded[i];
			coded[i] ^= 0x04;
			//coded[i] = coded[i]+1;
			//coded[i] = 0;
		}
		if ((i%22) == 0){
			//coded[i] = ~coded[i];
		}
		if ((i>430) && (i < 569)){
			coded[i] = ~coded[i];
		}
	}
	//the channel
	*/
	
	//random
	int r;
	for (i=0; i<enc->coded_bytes_len*enc->conv_blocks; i++){
		r = rand() % 100;
		if (r > 98){
			coded[i] = ~coded[i];
		}
		else if (r > 70) {
			coded[i] ^= 1 << (rand()%8);
		}
		
	}
	

	uint8_t * decoded = hdr_conv_encoder_decode(enc, coded, enc->coded_bytes_len*enc->conv_blocks);
	if (decoded == NULL){
		printf("Reed Solomon Decoding Failed\n");
		return -1;
	}
	assert(decoded != NULL);
	for (i=0, ober=0; i<enc->frame_bytes_len; i++) {
		if (decoded[i] != input[i]) {
			printf("%d\n",i);
			ober++;
		}
	}
	printf("FER....%d\n", ober);
	
	//printArrHex(input, enc->frame_bytes_len);
	//printArrHex(decoded, enc->frame_bytes_len);
	int rc = memcmp(decoded, input, enc->frame_bytes_len);
	if (rc == 0){
		printf("SUCCESS on type %d\n", type);
	} else {
		printf("FAILURE on type %d\n", type);
	}
	
	hdr_conv_encoder_destroy(&enc);
	return rc;
}

int main(void) {

	srand(time(NULL));   // Initialization, should only be called once.
	//srand(2);

	int i,rc;
	float iters = 1000;
	float s,f;
	float s2,f2;
	
	//for (i=0; i<(int)iters; i++){
	//	rc = error_test(GSM_xCCH);
	//	if (rc == 0){s++;}
	//	else {f++;}
	//}

	for (i=0; i<(int)iters; i++){
		rc = error_test(WiMax_FCH);
		if (rc == 0){s2++;}
		else {f2++;}
	}
	
	//printf("GSM_xCCH:  %2.0f successes, %2.0f failures, %3.1f%%\n", s, f, (s/iters)*100);
	printf("WiMax_FCH: %2.0f successes, %2.0f failures, %3.1f%%\n", s2, f2, (s2/iters)*100);
}
