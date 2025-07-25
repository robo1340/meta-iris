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
#include "header.h"

//int r = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

/*
struct __attribute__((__packed__)) frame_header_DEFINITION {
	uint8_t callsign[8]; ///<the src callsign of the frame
	uint16_t len; ///<len of the payload that is used
	uint16_t opt;
	uint32_t crc; ///<crc32 of the remaining payload
	uint8_t mac_tail[3]; ///<the last three bytes of the sending radio's MAC	
	uint8_t hdr_crc; ///<crc byte of the header
};
*/

int main(void) {
	conv_encoder_t * enc = hdr_conv_encoder();
	printf("\"%s\")\n", enc->enc->attr.name);
	printf("\"%s\" %d->%d\n", enc->enc->attr.spec, enc->enc->attr.in_len, enc->enc->attr.out_len);
	printf("frame len: %u\n", enc->frame_bytes_len);
	printf("%u -> %u bytes\n", enc->uncoded_bytes_len, enc->coded_bytes_len);
	printf("%u convolutional blocks\n", enc->conv_blocks);

	frame_header_t header = {
		.callsign = {0,1,2,3,4,5,6,7},
		.len = 2048,
		.opt = 0,
		.crc = 0,
		.mac_tail = {0xee, 0xff, 0xdd},
		.hdr_crc = 0
	};
	
	frame_header_t * header2;
	uint8_t * encoded;
	uint8_t channel[48];

	uint32_t len;
	encoded = encode_frame_header(enc, &header, &len);
	assert(encoded != NULL);
	assert(len == 48);
	
	memcpy(channel, encoded, len);
	
	header2 = decode_frame_header(enc, channel, len);
	assert(header2 != NULL);
	
	int rc = memcmp(&header, header2, sizeof(frame_header_t));
	assert(rc==0);
	
	//channel[4] = channel[4] ^= 1 << 2;
	channel[0] = 0;
	channel[1] = 0;
	header2 = decode_frame_header(enc, channel, len);
	assert(header2 == NULL);

	printf("Test passed!\n");
}
