

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <czmq.h>

#include "slip.h"

#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD	

int slip_encode(uint8_t * packet, uint32_t packet_len, uint8_t * slip_buf, uint32_t slip_buf_len) {
	uint32_t i; //packet index
	uint32_t j; //slip_buf index
	slip_buf[0] = SLIP_END; // Mark the packet end
	slip_buf_len-=2; //reduce perceived size of the buffer by 2 bytes as a safety precaution
	for (i=0,j=1; i<packet_len; i++) {
		if (j >= slip_buf_len) { // Check if we ran out of space on the slip_buf
			return -1;
		}

		switch (packet[i]) { // Grab one byte from the input and check if we need to escape it
			case SLIP_END:
				slip_buf[j] = SLIP_ESC;
				slip_buf[j + 1] = SLIP_ESC_END;
				j += 2;
				break;
			case SLIP_ESC:
				slip_buf[j] = SLIP_ESC;
				slip_buf[j + 1] = SLIP_ESC_ESC;
				j += 2;
				break;
			default:// No need to escape, copy as it is
				slip_buf[j] = packet[i];
				j += 1;
				break;
		}
	}

	slip_buf[j] = SLIP_END; // Mark the packet end
	return (j + 1); // Return the output size
}

#define SLIP_MAX_PACKET_SIZE 1500
#define SLIP_MAX_BUFFER_SIZE 2000


typedef struct rolling_buffer_DEF {
	uint8_t buf[SLIP_MAX_BUFFER_SIZE];
	uint8_t * ptr;
	uint32_t len;
}rolling_buffer_t;

static void rolling_buffer_init(rolling_buffer_t * rb){
	memset(rb->buf,0,sizeof(rb->buf));
	rb->ptr = rb->buf;
	rb->len = 0;
}

#define rolling_buffer_maxlen(rb) (sizeof(rb->buf))
#define rolling_buffer_len(rb) (rb->len)

static bool rolling_buffer_append(rolling_buffer_t * rb, uint8_t * to_append, uint32_t len){
	if ((rb->len + len) > rolling_buffer_maxlen(rb)) {
		printf("WARNING: rolling_buffer_append overflow!\n");
		rolling_buffer_init(rb);
		if (len > rolling_buffer_maxlen(rb)){
			printf("ERROR: append bytes %u exceeds buffer size\n", len);
			return false;
		}
	}
	memcpy(rb->ptr, to_append, len);
	rb->ptr += len;
	rb->len += len;
	return true;
}

static bool rolling_buffer_shift_left(rolling_buffer_t * rb, uint32_t shift_amount){
	if ((shift_amount > rolling_buffer_len(rb)) || (shift_amount > rolling_buffer_maxlen(rb))) {
		rolling_buffer_init(rb);
		return true;
	}
	rb->len -= shift_amount;
	memmove(rb->buf, &rb->buf[shift_amount], rb->len);
	rb->ptr = rb->buf + rb->len;
	return true;
}

static const uint8_t slip_end[1] = {SLIP_END};
static const uint8_t slip_esc[1] = {SLIP_ESC};

zchunk_t * slip_decoder_feed(uint8_t * input, size_t len){
	static bool buf_init = false;
	static rolling_buffer_t buf;
	static zchunk_t * packet  	= NULL; //received packet buffer
	zchunk_t * to_return = NULL;
	uint32_t i,j; //j is i+1
	
	//if (len == 0){return NULL;} //protect against zero length input
	
	if (!buf_init) {
		rolling_buffer_init(&buf);
		buf_init = true;
	}
	
	if (packet == NULL){
		packet = zchunk_new(NULL,SLIP_MAX_PACKET_SIZE);
		if (packet==NULL) {
			printf("ERROR: out of memory in slip_decoder_feed()\n");
			return NULL;
		}
	}
	
	if (!rolling_buffer_append(&buf, input, len)){
		return NULL;
	}
	
	#define current buf.buf[i]
	#define next buf.buf[j]

	for (i=0,j=1; i<buf.len; i++, j++){
		if (current == SLIP_ESC) {
			if (j >= buf.len){ //next character in escape sequence hasn't been fed yet
				break;
			}
			if (next == SLIP_ESC_END) {
				zchunk_append(packet, slip_end, sizeof(slip_end));
			} else if (next == SLIP_ESC_ESC){
				zchunk_append(packet, slip_esc, sizeof(slip_esc));
			} else { // Escape sequence invalid, complain on stderr
				zchunk_append(packet, slip_esc, sizeof(slip_esc));
				printf("WARNING SLIP escape error! (Input bytes 0x%02x, 0x%02x)\n", current, next);
			}
			i++; j++;
		} else if (current == SLIP_END) {
			if (zchunk_size(packet) > 0){ // End of packet, stop the loop
				to_return = packet; //set to return the packet
				packet = NULL; //create a new packet buffer next time this function is called
				break;
			} //otherwise this was just the beginning of the packet
		} else { //append byte to packet
			zchunk_append(packet, &current, 1);
			//printf("append %x\n", current);
		}
	}
	
	//take bytes out of the buffer that have been processed
	if (!rolling_buffer_shift_left(&buf, i)){
		return NULL;
	}
	
	return to_return;
}

/*
//int slip_decoder_feed(uint8_t in_byte, uint8_t * packet) {
uint8_t * slip_decoder_feed(uint8_t in_byte, int * packet_len) {
	static uint8_t packet_buf[1580];
	static uint8_t * packet_buf_pos = packet_buf;
	static slip_bytes_t b;
	static bool decoder_started = false;
	
	*packet_len = 0;
	
	if (!decoder_started){
		b.current= in_byte;
		decoder_started = true;
		return NULL;
	} else if ((uint32_t)(packet_buf_pos-packet_buf) > sizeof(packet_buf)){ //check for overflow
		printf("WARNING: buffer overflow in slip_decode()\n");
		decoder_started = false;
		packet_buf_pos = packet_buf;
		*packet_len = -1;
		return NULL;
	}
	
	b.next = in_byte;
	
	if (b.current == SLIP_ESC) {
		switch (b.next) {
			case SLIP_ESC_END:
				*packet_buf_pos = SLIP_END; 
				packet_buf_pos++;
				break;
			case SLIP_ESC_ESC:
				*packet_buf_pos = SLIP_ESC; 
				packet_buf_pos++;
				break;
			default: // Escape sequence invalid, complain on stderr
				*packet_buf_pos = SLIP_ESC; 
				packet_buf_pos++;
				printf("WARNING SLIP escape error! (Input bytes 0x%02x, 0x%02x)\n", b.current, b.next);
				break;
		}
	} else if (b.current == SLIP_END) {// End of packet, stop the loop
		*packet_len = (packet_buf_pos-packet_buf);
		packet_buf_pos = packet_buf;
	} else {
		*packet_buf_pos = b.current; 
		packet_buf_pos++;
	}
	b.current = b.next;
	
	if (*packet_len > 0){
		return packet_buf;
	}
	return NULL;
}
*/


