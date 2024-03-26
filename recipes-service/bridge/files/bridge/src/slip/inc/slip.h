#ifndef _SLIP_H
#define _SLIP_H

#include <stdint.h>
#include <czmq.h>

/** @brief encode a packet into a slip frame
 *  @param packet pointer to the ipv4 packet to encode
 *  @param packet_len length of the packet
 *  @param slip_buf output buffer where the slip frame will be stored
 *  @param slip_buf_len length of the slip frame buffer
 *  @return returns the length of the slip_buffer used on success, returns -1 on failure
 */
int slip_encode(uint8_t * packet, uint32_t packet_len, uint8_t * slip_buf, uint32_t slip_buf_len);

/** @brief feed bytes to a slip frame decoder machine
 *  @param input pointer to the input byte array
 *  @param len length of the input byte array
 *  @return returns a new zchunk_t containing a decoded packet on success, returns NULL otherwise
 */
zchunk_t * slip_decoder_feed(uint8_t * input, size_t len);

#endif 






