#ifndef _HEADER_H
#define _HEADER_H
#endif 

#include "state.h"

#include <stdint.h>
#include <stdbool.h>

#define ENC_HEADER_LEN_BYTES 48//after 1:2 coding added header is 48 bytes long

frame_header_t * decode_frame_header(conv_encoder_t * encoder, uint8_t * to_decode, uint32_t len);
uint8_t * encode_frame_header(conv_encoder_t * encoder, frame_header_t * to_encode, uint32_t * encoded_len);

