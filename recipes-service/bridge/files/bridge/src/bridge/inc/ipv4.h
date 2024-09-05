#ifndef _IPV4_H_
#define _IPV4_H_

#include "state.h"

#include <stdint.h>
#include <stdbool.h>

///packed struct definition of the first few field of an IPV4 header assuming the Internet Header Length is set to 5
///note that the individual field will need to be flipped from little endian to big endian to see their true values
typedef struct __attribute__((__packed__)) ipv4_header_DEF {
	uint8_t version_ihl; ///<version (4b) and IHL (4b) fields
	uint8_t dscp_ecn; ///<Differentiated Services Code Point (6b) and ECN(2) fields
	uint16_t total_length; ///<total length of the packet
	uint16_t identification; ///<identification field for fragmented packets
	uint16_t flags_fragment_offset; ///<flags(3b) and fragment offset(13b) fields
	uint8_t ttl; ///<time to live field
	uint8_t protocol; ///<protocol field
	uint16_t header_checksum; ///<header checksum field
	uint32_t src; ///<source address field
	uint32_t dst; ///<destination address field
	uint8_t ip_data; ///<first byte of IP data
} ipv4_hdr_t;

typedef struct __attribute__((__packed__)) udp_datagram_DEF {
	uint8_t version_ihl; ///<version (4b) and IHL (4b) fields
	uint8_t dscp_ecn; ///<Differentiated Services Code Point (6b) and ECN(2) fields
	uint16_t total_length; ///<total length of the packet
	uint16_t identification; ///<identification field for fragmented packets
	uint16_t flags_fragment_offset; ///<flags(3b) and fragment offset(13b) fields
	uint8_t ttl; ///<time to live field
	uint8_t protocol; ///<protocol field
	uint16_t header_checksum; ///<header checksum field
	uint32_t src; ///<source address field
	uint32_t dst; ///<destination address field
	uint16_t src_port; ///<source UDP port
	uint16_t dst_port; ///<destination UDP port
	uint16_t udp_len; ///<the UDP packet length
	uint16_t udp_checksum; ///<the UDP checksum
	uint8_t  udp_data; ///<first byte of UDP data
} udp_datagram_t;


/** @brief check if the destination of an ipv4 packet is addressed to this node
 *  @param state global state object
 *  @param ipv4 pointer to the ipv4 header
 *  @return return true if the destination ip address of ipv4 matches this node's ip address, returns false otherwise
 */
bool ipv4_check_dst(state_t * state, ipv4_hdr_t * ipv4);

/** @brief decrement the ttl of an ipv4 header
 *  @param state global state pointer
 *  @param ipv4 packet to decrement
 *  @return returns true if the ipv4 packet should be retransmitted, returns false otherwise
 */
bool ipv4_ttl_decrement(state_t * state, ipv4_hdr_t * ipv4);

void ipv4_print(ipv4_hdr_t * ipv4);

//filter packets before placing on the radio interface
bool ipv4_filter_misc(ipv4_hdr_t * ipv4);


// set ip checksum of a given ip header
void ipv4_recompute_checksum(ipv4_hdr_t * ipv4);

void ipv4_set_ttl(ipv4_hdr_t * ipv4, uint8_t new_ttl);

/** @brief check if an ipv4 packet is UDP and uses the port tagged for low priority_queue
    @param ipv4 pointer to the ipv4 packet
	@param uint16_t the low priority port number
	@return returns true if the ipv4 packet is low priority UDP, returns false otherwise
*/
bool check_udp_dst_port(udp_datagram_t * udp, uint16_t port);

#endif
