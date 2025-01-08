#include "ipv4.h"
#include "print_util.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define ALLOW_BROADCAST

static bool compute_udp_checksum(udp_datagram_t * udp);

bool ipv4_check_dst(state_t * state, ipv4_hdr_t * ipv4){
	
	if ((ipv4->version_ihl & 0xF0) != 0x40){
		//printf("not ipv4 %x\n",ipv4->version_ihl);
		return false;
	}
	
	//return true if the first two bytes of the destination IP address matches the address of this node
	if (memcmp(state->ip, &ipv4->dst, 3)==0) {
		return true;
	}
#ifdef ALLOW_BROADCAST
	else if ((((uint8_t*)&ipv4->dst)[1]==255)&&(((uint8_t*)&ipv4->dst)[2]==255)) {
		printf("broadcast\n");
		memcpy(&ipv4->dst, state->ip, 3); //change the destination IP address first three bytes
		ipv4_recompute_checksum(ipv4);
		((udp_datagram_t*)ipv4)->udp_checksum = 0x0000; //set the UDP checksum to 0 (not used)
		return true;
	}
#endif
	return false;
}

#define UDP_PROTO 0x11

//check if a packet contains a UDP datagram and recompute the check sum if sopen
//returns false if packet is not a UDP datagram
//NOT WORKING
static bool compute_udp_checksum(udp_datagram_t * udp) {
	if (udp->protocol != UDP_PROTO) {return false;}
	
	uint32_t sum = 0;
	uint16_t udp_len = htons(udp->udp_len);
	uint8_t * ip_pay = &(((ipv4_hdr_t*)udp)->ip_data);
	
	//add the pseudo header 
	sum += (udp->src>>16) & 0xFFFF; //the source ip
	sum += (udp->src) & 0xFFFF;
	sum += (udp->dst>>16) & 0xFFFF; //the dest ip
	sum += (udp->dst) & 0xFFFF;
	sum += (uint16_t)UDP_PROTO; //protocol and reserved: 17
	sum += udp->udp_len; //the length
	
	udp->udp_checksum = 0x0000; //initialize checksum to 0
	while (udp_len > 1) {
		sum += *((uint16_t*)ip_pay);
		ip_pay += 2;
		udp_len -= 2;
	}
	if(udp_len > 0) { //if any bytes left, pad the bytes and add
		sum += ((*((uint16_t*)ip_pay)) & htons(0xFF00));
	}
	
	while (sum>>16) { //Fold sum to 16 bits: add carrier to result
		sum = (sum & 0xffff) + (sum >> 16);
	}
	sum = ~sum;
	//set computation result
	udp->udp_checksum = ((uint16_t)sum == 0x0000) ? 0xFFFF: (uint16_t)sum;
	return true;
}

// Compute checksum for count bytes starting at addr, using one's complement of one's complement sum
static uint16_t compute_checksum(uint16_t * addr, uint32_t count) {
	uint32_t sum = 0;
	while (count > 1) {
		sum += *addr++;
		count -= 2;
	}
	if(count > 0) { //if any bytes left, pad the bytes and add
		sum += ((*addr)&htons(0xFF00));
	}
	while (sum>>16) { //Fold sum to 16 bits: add carrier to result
		sum = (sum & 0xffff) + (sum >> 16);
	}
	sum = ~sum; //one's complement
	return ((uint16_t)sum);
}

void ipv4_recompute_checksum(ipv4_hdr_t * ipv4){
	ipv4->header_checksum = 0;
	ipv4->header_checksum = compute_checksum((uint16_t*)ipv4, (ipv4->version_ihl&0x0F)<<2);
}

void ipv4_set_ttl(ipv4_hdr_t * ipv4, uint8_t new_ttl){
	ipv4->ttl = new_ttl;
	ipv4_recompute_checksum(ipv4);
}

bool ipv4_ttl_decrement(state_t * state, ipv4_hdr_t * ipv4){
	uint16_t ttl = ntohs(ipv4->ttl);
	uint16_t checksum = ntohs(ipv4->header_checksum);
	uint32_t sum;
	if (ttl==0){return false;}
	
	//decrement time to live and incrementally recompute the checksum
	ttl--;
	sum = checksum + 0x0100; //increment checksum high byte
	checksum = (sum + (sum>>16)); //add carry
	
	//place ttl and checksum back in packet in network byte order
	ipv4->header_checksum = htons(checksum);
	ipv4->ttl = htons(ttl);
	return true;
}

static char * addr_str(uint32_t addr){
	static char to_return[16];
	uint8_t * p = (uint8_t*)&addr;
	snprintf(to_return, sizeof(to_return), "%u.%u.%u.%u", p[0],p[1],p[2],p[3]);
	return to_return;
}

static char * addr_str_buf(uint32_t addr, char * buf, size_t buf_len){
	uint8_t * p = (uint8_t*)&addr;
	snprintf(buf, buf_len, "%u.%u.%u.%u", p[0],p[1],p[2],p[3]);
	return buf;
}

void ipv4_print(ipv4_hdr_t * ipv4){
	char src[16];
	char dst[16];
	addr_str_buf(ipv4->src, src, sizeof(src)),
	addr_str_buf(ipv4->dst, dst, sizeof(dst)),
	printf("IP %s->%s, len=%u, ttl=%u, proto=%x, checksum=0x%04x\n", 
		src, 
		dst, 
		ntohs(ipv4->total_length), 
		ipv4->ttl, 
		ipv4->protocol,
		ntohs(ipv4->header_checksum)
	);
}

//deprecated!!
bool ipv4_filter_misc(ipv4_hdr_t * ipv4){
	static uint8_t mdns_octet_1 = 224;
	
	if (memcmp(&ipv4->src, &mdns_octet_1, 1)==0) {
		return false;
	}
	if (memcmp(&ipv4->dst, &mdns_octet_1, 1)==0) {
		return false;
	}
	
	return true;
}

bool check_udp_dst_port(udp_datagram_t * udp, uint16_t port){
	if (udp->protocol != UDP_PROTO) {return false;}
	//printf("%u %u\n", ntohs(udp->src_port), ntohs(udp->dst_port)); 
	return (ntohs(udp->dst_port) == port);
}
