//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "conv_wrapper.h"
#include "print_util.h"

//int r = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

/* Bit error rate test */
static int error_test(uint8_t type){
	int i, ober = 0;
	printf("\nerror_test(%u,", type);
	
	encoder_t * enc = encoder(type);
	assert(enc != NULL);
	printf("\"%s\")\n", enc->attr.name);
	printf("\"%s\" %d->%d\n", enc->attr.spec, enc->attr.in_len, enc->attr.out_len);


	uint8_t * input = malloc(enc->uncoded_bytes_len);

	for (i=0; i<enc->uncoded_bytes_len; i++){
		enc->uncoded_bytes[i] = i%255;
		input[i] = i%255;
	}

	uint8_t * coded = encoder_encode(enc, enc->uncoded_bytes, enc->uncoded_bytes_len);
	assert(coded != NULL);

	//send over channel, simulate errors
	for (i=0; i<enc->coded_bytes_len; i++){
		if ((i%3) == 0){
			//coded[i] = ~coded[i];
			coded[i] ^= 0x04;
			//coded[i] = coded[i]+1;
			//coded[i] = 0;
		}
	}
	//the channel

	uint8_t * decoded = encoder_decode(enc, coded, enc->coded_bytes_len);
	assert(decoded != NULL);
	for (i=0, ober=0; i<enc->uncoded_bytes_len; i++) {
		if (decoded[i] != input[i]) {ober++;}
	}
	printf("FER....%d\n", ober);

	printArrHex(input, enc->uncoded_bytes_len);
	printArrHex(decoded, enc->uncoded_bytes_len);
	int rc = memcmp(decoded, input, enc->uncoded_bytes_len);
	if (rc == 0){
		printf("SUCCESS on type %d\n", type);
	} else {
		printf("FAILURE on type %d\n", type);
	}
	
	encoder_destroy(&enc);
	return 0;
}

int main(void) {

	error_test(GSM_xCCH);
	error_test(GPRS_CS2);
	error_test(GPRS_CS3);
	error_test(GSM_RACH);
	error_test(GSM_SCH);
	error_test(GSM_TCH_FR);
	error_test(GSM_TCH_AFS_12_2);
	error_test(GSM_TCH_AFS_10_2);
	error_test(GSM_TCH_AFS_7_95);
	error_test(GSM_TCH_AFS_7_4);
	error_test(GSM_TCH_AFS_6_7);
	error_test(GSM_TCH_AFS_5_9);
	error_test(GSM_TCH_AHS_7_95);
	error_test(GSM_TCH_AHS_7_4);
	error_test(GSM_TCH_AHS_6_7);
	error_test(GSM_TCH_AHS_5_9);
	error_test(GSM_TCH_AHS_5_15);
	error_test(GSM_TCH_AHS_4_75);
	error_test(WiMax_FCH);
	error_test(LTE_PBCH);

	printf("Test passed!\n");
}
