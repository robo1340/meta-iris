//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "print_util.h"
#include "interleaver.h"

//int r = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

/* Bit error rate test */
static int error_test(uint16_t type){
	int i,rc;
	printf("\nerror_test(%u)\n", type);
	
	
	uint8_t tv[type];
	for (i=0;i<sizeof(tv);i++){
		tv[i] = i;
	}
	uint8_t tv_int[sizeof(tv)];
	uint8_t tv_out[sizeof(tv)];

	turbo_interleaver_init();
	
	rc = turbo_interleave(sizeof(tv), tv, tv_int);
	assert(rc==0);
	
	//printArrHex(tv, sizeof(tv));
	//printArrHex(tv_int, sizeof(tv_int));

	rc = turbo_deinterleave(sizeof(tv), tv_int, tv_out);
	assert(rc==0);
	
	//printArrHex(tv_out, sizeof(tv_out));

	rc = memcmp(tv, tv_out, sizeof(tv));
	if (rc == 0){
		printf("SUCCESS on type %d\n", type);
	} else {
		printf("FAILURE on type %d\n", type);
	}
	
	turbo_interleaver_deinit();
	return 0;
}

int main(void) {


	uint16_t input_lengths[] = {  40,  48,
								  56,  64,
								  72,  80,
								  88,  96,
								 104, 112,
								 120, 128,
								 136, 144,
								 152, 160,
								 168, 176,
								 184, 192,
								 200, 208,
								 216, 224,
								 232, 240,
								 248, 256,
								 264, 272,
								 280, 288,
								 296, 304,
								 312, 320,
								 328, 336,
								 344, 352,
								 360, 368,
								 376, 384,
								 392, 400,
								 408, 416,
								 424, 432,
								 440, 448,
								 456, 464,
								 472, 480,
								 488, 496,
								 504, 512,
								 528, 544,
								 560, 576,
								 592, 608,
								 624, 640,
								 656, 672,
								 688, 704,
								 720, 736,
								 752, 768,
								 784, 800,
								 816, 832,
								 848, 864,
								 880, 896,
								 912, 928,
								 944, 960,
								 976, 992,
								1008,1024,
								1056,1088,
								1120,1152,
								1184,1216,
								1248,1280,
								1312,1344,
								1376,1408,
								1440,1472,
								1504,1536,
								1568,1600,
								1632,1664,
								1696,1728,
								1760,1792,
								1824,1856,
								1888,1920,
								1952,1984,
								2016,2048,
								2112,2176,
								2240,2304,
								2368,2432,
								2496,2560,
								2624,2688,
								2752,2816,
								2880,2944,
								3008,3072,
								3136,3200,
								3264,3328,
								3392,3456,
								3520,3584,
								3648,3712,
								3776,3840,
								3904,3968,
								4032,4096,
								4160,4224,
								4288,4352,
								4416,4480,
								4544,4608,
								4672,4736,
								4800,4864,
								4928,4992,
								5056,5120,
								5184,5248,
								5312,5376,
								5440,5504,
								5568,5632,
								5696,5760,
								5824,5888,
								5952,6016,
								6080,6144};

	int i;
	for (i=0; i<sizeof(input_lengths)/2; i++){
		error_test(input_lengths[i]);
	}

	printf("Test passed!\n");
}
