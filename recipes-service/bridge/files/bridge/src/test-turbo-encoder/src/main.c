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
static int error_test(turbo_encoder_t * enc){
	int i, ober = 0;
	//printf("error_test(");
	//printf("\"%s\")\n", enc->attr.name);
	//printf("\"%s\" %d->%d\n", enc->attr.spec, enc->attr.in_len, enc->attr.out_len);
	
	uint8_t input[768];
	//uint8_t * input = malloc(enc->frame_bytes_len);

	for (i=0; i<enc->frame_bytes_len; i++){
		input[i] = rand();
	}
	//printArrHex(input, enc->frame_bytes_len);

	uint8_t * coded = turbo_encoder_encode(enc, input, enc->frame_bytes_len);
	assert(coded != NULL);

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
	for (i=0; i<enc->coded_bytes_len; i++){
		r = rand() % 100;
		if (r > 97){
			coded[i] = ~coded[i];
		}
		else if (r > 60) {
			coded[i] ^= 1 << (rand()%8);
		}
		
	}
	

	uint8_t * decoded = turbo_encoder_decode(enc, coded, enc->coded_bytes_len);
	if (decoded == NULL){
		//printf("Reed Solomon Decoding Failed\n");
		return -1;
	}
	//assert(decoded != NULL);
	for (i=0, ober=0; i<enc->frame_bytes_len; i++) {
		if (decoded[i] != input[i]) {
			printf("%d\n",i);
			ober++;
		}
	}
	
	if (ober != 0){
		printf("FER....%d\n", ober);
	}

	//printArrHex(input, enc->frame_bytes_len);
	//printArrHex(decoded, enc->frame_bytes_len);
	int rc = memcmp(decoded, input, enc->frame_bytes_len);
	if (rc == 0){
		//printf("SUCCESS\n");
	} else {
		//printf("FAILURE\n");
	}
	
	return rc;
}

typedef struct test_vector_t {
	uint16_t block_size;
	uint16_t rs_block_size;
	int iterations;
	float s;
	float f;
} test_vector_t;

test_vector_t tvs[] = {
	//{3072, 4, 8, 0, 0},
	//{3072, 8, 8, 0, 0},
	//{3072, 12, 8, 0, 0},
	//{768, 24, 2, 0, 0},
	//{6144, 24, 2, 0, 0},
	{3072, 24, 2, 0, 0},
	{3072, 48, 2, 0, 0},
	{3072, 96, 2, 0, 0},
	{3072, 192, 2, 0, 0},
};

int main(void) {
	int seed = 42;
	srand(time(NULL));   // Initialization, should only be called once.
	//srand(2);

	int i,j,rc;
	float iters = 25;
	turbo_encoder_t * enc = NULL;
	
	for (j=0; j<sizeof(tvs)/sizeof(test_vector_t); j++){
		srand(seed);
		
		enc = turbo_encoder(tvs[j].block_size, tvs[j].rs_block_size, tvs[j].iterations); 
		assert(enc != NULL);
		printf("frame len: %u\n", enc->frame_bytes_len);
		printf("%u -> %u bytes\n", enc->uncoded_bytes_len, enc->coded_bytes_len);
		
		for (i=0; i<(int)iters; i++){
			rc = error_test(enc);
			if (rc == 0){tvs[j].s++;}
			else {tvs[j].f++;}
		}
		turbo_encoder_destroy(&enc);
	}
	
	for (j=0; j<sizeof(tvs)/sizeof(test_vector_t); j++){
		printf("(%u,%u,%d) %2.0f successes, %2.0f failures, %3.1f%%\n", tvs[j].block_size, tvs[j].rs_block_size, tvs[j].iterations, tvs[j].s, tvs[j].f, (tvs[j].s/iters)*100);
	}
}
