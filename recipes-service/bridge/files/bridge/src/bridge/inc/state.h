#ifndef state_H_
#define state_H_

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <inttypes.h>
#include <zmq.h>
#include <czmq.h>

//max tx queue wait time is a=1000, minimum is b=50
//TRANSMIT_SEND_QUEUE=((a-b)/2)+a, TRANSMIT_SEND_QUEUE_VARY_MS=a-TRANSMIT_SEND_QUEUE

//default settings
#define DEFAULT_TTL_SET   					  64	///<the ttl value to set for all slip packets coming from the local network	
#define DEFAULT_TTL_DROP  					  60   ///<the ttl value at which to drop packets coming from the radio network
#define DEFAULT_BEACON_PERIOD_MS			  30000///<the nominal period at which to transmit a link beacon
#define DEFAULT_BEACON_PERIOD_VARIANCE_MS	  3000 ///<the upper and lower bounds to be randomly added to the beacon period
#define DEFAULT_BEACON_TTL					  3    ///<the time to live value for link beacons
#define DEFAULT_TRANSMIT_QUEUE_PERIOD_MS	  525  ///<the nominal period at which to transmit radio frames in the send queue
#define DEFAULT_TRANSMIT_QUEUE_VARIANCE_MS	  475  ///<the upper and lower bound to be randomly added to the send queue
#define DEFAULT_LINK_PEER_PERIOD_MS			  60000///<the nominal period at which to send link peer notifications
#define DEFAULT_LINK_PEER_VARIANCE_MS		  6000 ///<the upper and lower bound to be randomly added to the link peer notifications period
#define DEFAULT_LINK_PEER_TTL				  3    ///<the time to live value for link peer notifications
#define DEFAULT_LINK_PEER_TIMEOUT_MS		  180000///<the timeout value for individual link peers, at which point they will be removed from the link peer list
#define DEFAULT_USER_PERIOD_MS				  60000 //<the nominal period at which to send link user notifications
#define DEFAULT_USER_VARIANCE_MS			  6000 ///<the upper and lower bound for link user notifications
#define DEFAULT_USER_TTL					  3    ///<the time to live value for link user notifications
#define DEFAULT_NICENESS					  0     ///<the niceness value to apply to the program from -20 to 20
#define DEFAULT_MAX_LOADING_DOCK_IDLE_MS	  100	///<the time a non-empty transmit loading dock will wait for additional packets before being added to the transmit queue
#define RADIO_DEFAULT_CHANNEL				  0     ///<the default radio channel to use
#define DEFAULT_COMPRESS_IPV4				  true ///<set to true to compress ipv4 packets
#define DEFAULT_SLIP_MTU					  500   ///<MTU for the slip interface
#define DEFAULT_WIFI_SSID					  "irisnode"   ///<default SSID for the wifi AP
#define DEFAULT_WIFI_PASSPHRASE				  "irisnode"   ///<default passphrase for the wifi AP
#define LOW_PRIORITY_UDP_PORT				  1337 ///<UDP port on which send datagrams will be treated as a low priority packet

//global state settings
#define MAX_PACKET_LEN 747 ///<the maximum length of packet that can be transmitted over the radio

#define RADIO_MAX_PACKET_LENGTH     SI466X_FIFO_SIZE ///< Maximal SI4463 packet length definition (FIFO size)
#define RADIO_TX_ALMOST_EMPTY_THRESHOLD 64u ///<Threshold for TX FIFO
#define RADIO_RX_ALMOST_FULL_THRESHOLD  64u ///<Threshold for RX FIFO

//convenience macros
#define RANDINT(lower, upper) ((rand() % (upper-lower+1)) + lower)

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////high level object typedefs start////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
typedef struct state_t_DEFINITION state_t;

typedef struct frame_header_DEFINITION frame_header_t; 	///<frame header
typedef struct tlv_DEFINITION tlv_t; 					///<definition for a sub-payload making up a part of the payload of a frame
typedef struct frame_DEFINITION frame_t; 				///<frame object definition before compression and packing
typedef struct packed_frame_DEFINITION packed_frame_t; 	///<frame packed and compressed before turbo encoding
typedef struct radio_frame_t_DEFINITION radio_frame_t; 	///<frame after turbo encoding
typedef struct peer_t_DEFINITION peer_t;				///<object to hold info on a connection to a peer station
typedef struct beacon_DEFINITION beacon_t;				///<object to hold information in a radio beacon


////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////global object declarations////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
extern state_t * global_state; ///<global reference to the state_t, use with caution

typedef struct csma_DEF {
	bool enabled; 			///<csma is active
	uint8_t tx_probability; ///<the probability to transmit, a value from 0% to 100%
	uint16_t wait_upper;	///<upper limit in ms on the wait period
	uint16_t wait_lower; 	///<lower limit in ms on the wait period
	uint16_t period_ms;		///<current wait period in ms
	int64_t count_ms;		///<the current count in millis, -1 when timer not set
}csma_t;

typedef struct timer_DEF {
	bool enabled;
	uint32_t period_ms;
	int64_t count_ms;
	int16_t stachastic_ms; //used to vary the period
}task_timer_t;

typedef struct timed_frame_DEFINITION {
	int64_t created_ms; ///<the time frame was created in ms
	int64_t max_idle_ms;///<the maximum time the frame will idle before being added to the send queue
	frame_t * frame;	
} timed_frame_t;

//example config
/*
{
	"ttl_set" 						: 64,	#the ttl value to set for all slip packets coming from the local network					
	"ttl_drop"						: 60,   #the ttl value at which to drop packets coming from the radio network
	"beacon_period_ms" 				: 30000,#the nominal period at which to transmit a link beacon
	"beacon_period_variance_ms" 	: 3000, #the upper and lower bounds to be randomly added to the beacon period
	"beacon_ttl" 					: 3,    #the time to live value for link beacons
	"transmit_queue_period_ms" 		: 525,  #the nominal period at which to transmit radio frames in the send queue
	"transmit_queue_variance_ms" 	: 475,  #the upper and lower bound to be randomly added to the send queue
	"link_peer_period_ms" 			: 60000,#the nominal period at which to send link peer notifications
	"link_peer_variance_ms" 		: 6000, #the upper and lower bound to be randomly added to the link peer notifications period
	"link_peer_ttl" 				: 3,    #the time to live value for link peer notifications
	"link_peer_timeout_ms" 			: 180000,#the timeout value for individual link peers, at which point they will be removed from the link peer list
	"user_period_ms" 				: 60000, #the nominal period at which to send link user notifications
	"user_variance_ms" 				: 6000,  #the upper and lower bound for link user notifications
	"user_ttl" 						: 3,     #the time to live value for link user notifications
	"niceness"						: 0,      #the niceness value to apply to the program from -20 to 20
	"compress_ipv4"					: true,	 #set to true to compress ipv4 packets
	"slip_mtu"						: 500,	#MTU for the slip interface 
	"wifi_ssid"						: "irisnode",
	"wifi_passphrase"				: "irisnode",
	"low_priority_udp_port"			: 1337
}
*/

///typedef struct to contain the current state of the event_detector
struct state_t_DEFINITION {
	//////////////////////////////////////////
	//Settings retrieved from a config file
	//////////////////////////////////////////
	uint8_t ip[4]; 		///<IP address of the radio interface
	char ip_str[20]; 	///<IP address of the radio interface
	uint8_t mac[6]; 	///<MAC address of the radio interface
	char mac_str[20]; 	///<MAC address of the radio interface
	char callsign[8]; 	///<callsign of the radio interface
	//config.json settings
	uint16_t ttl_set;   				///<the ttl value to set for all slip packets coming from the local network	
	uint16_t ttl_drop;  				///<the ttl value at which to drop packets coming from the radio network
	uint32_t beacon_period_ms;			///<the nominal period at which to transmit a link beacon
	uint32_t beacon_period_variance_ms;	///<the upper and lower bounds to be randomly added to the beacon period
	uint8_t  beacon_ttl;				///<the time to live value for link beacons
	uint16_t transmit_queue_period_ms;	///<the nominal period at which to transmit radio frames in the send queue
	uint16_t transmit_queue_variance_ms;///<the upper and lower bound to be randomly added to the send queue
	uint32_t link_peer_period_ms;		///<the nominal period at which to send link peer notifications
	uint32_t link_peer_variance_ms;		///<the upper and lower bound to be randomly added to the link peer notifications period
	uint8_t  link_peer_ttl;				///<the time to live value for link peer notifications
	uint32_t link_peer_timeout_ms;		///<the timeout value for individual link peers, at which point they will be removed from the link peer list
	uint32_t user_period_ms;			///<the nominal period at which to send link user notifications
	uint32_t user_variance_ms;			///<the upper and lower bound for link user notifications
	uint8_t  user_ttl;					///<the time to live value for link user notifications
	int8_t   niceness;					///<the niceness value to apply to the program from -20 to 20
	uint16_t max_loading_dock_idle_ms;	///<the time a non-empty transmit loading dock will wait for additional packets before being added to the transmit queue
	uint8_t  current_channel;			///<the currently selected radio channel, defaults to RADIO_DEFAULT_CHANNEL
	bool     compress_ipv4;				///<set to true to compress ipv4 packets
	uint16_t slip_mtu;					///<MTU for the slip interface 
	char wifi_ssid[64];					///<wifi AP SSID
	char wifi_passphrase[64];			///<wifi AP passphrase
	zlistx_t * si4463_load_order; ///<a list of si4463 config settings to load first
	uint16_t low_priority_udp_port;     ///<UDP port on which low priority datagrams are sent

	//settings derives from config contents
	uint8_t tx_backoff_min_ms;

	//si4463 config settings
	zhashx_t * radio_config; ///<keys are defines from a radio config, values are zchunk_t containing the definition
	
	//////////////////////////////////////////
	//Dynamic State Variables
	//////////////////////////////////////////	
	int slip; 			///<file descriptor for a slip serial port
	char slip_if[8];
	char serial_path[12];
	
	timed_frame_t loading_dock; ///<the current frame being filled with payload data that will be added to the send queue on a counter or when full
	
	radio_frame_t * receiving;	 ///<not equal to null when a radio frame is being received	
	radio_frame_t * transmitting;///<not equal to null when a radio frame is transmitting
	
	zlistx_t * send_queue; ///<a list of radio_frame_t to be sent
	
	zlistx_t * peers; 				///<a list of peers that we have had contact with direct or indirect
	zlistx_t * link_peers; 			///<a list of peers that we have a direct radio link to, uses pointers from peers
	zhashx_t * peers_by_ip;			///<hash table of peers where the key is IP string
	zhashx_t * peers_by_mac;		///<hash table of peers where the key is MAC string
	zhashx_t * peers_by_callsign;	///<hash table of peers where the key is callsign string
	
	zlistx_t * low_priority_queue;  ///<a list of zchunk_t that are low priority packets to send
	
	task_timer_t beacon_transmit;
	task_timer_t transmit_send_queue;
	csma_t csma;
	
};

state_t * state(void);

bool state_restart_wifi_ap(state_t * state);

/** @brief process a newly received frame using the current state as context
 *  @param state pointer to the state object
 *  @param received_frame double pointer to the received frame, will be destroyed when processed
 *  @return returns true when received_frame was successfully processed, returns false otherwise
 */
bool state_process_receive_frame(state_t * state, frame_t ** received_frame);

/** @brief update the peers lists in the state object with a newly received header
 *  this function will update/append an entry in state->peers and will update the entries in 
 *  link_peers, peers_by_ip, peers_by_mac, and peers_by_callsign as needed
 *  @brief state the state object instance
 *  @brief hdr a newly received header from an existing or new peer, set to NULL if this argument is not used
 *  @return return true on success, returns false otherwise
 */
bool state_update_peers_new_header(state_t * state, frame_header_t * hdr);

/** @brief update the peers lists in the state object with a newly received beacon
 *  this function will update/append an entry in state->peers and will update the entries in 
 *  link_peers, peers_by_ip, peers_by_mac, and peers_by_callsign as needed
 *  @brief state the state object instance
 *  @brief b a newly received beacon from an existing or new peer, set to NULL if this argument is not used
 *  @return return true on success, returns false otherwise
 */
bool state_update_peers_new_beacon(state_t * state, beacon_t * b);

bool state_transmitting(state_t * state);
bool state_receiving(state_t * state);
bool state_transceiving(state_t * state);

/** @brief tracks when it is time to send off the data on the loading dock
 *  @return returns false on failure
 */
bool state_loading_dock_run(state_t * state); 
#define state_loading_dock_empty(state) (state->loading_dock.created_ms < 0)
bool state_append_loading_dock(state_t * state, uint8_t type, uint16_t len, uint8_t * value);
bool state_append_low_priority_packet(state_t * state, uint8_t type, uint16_t len, uint8_t * value);
bool state_add_frame_to_send_queue(state_t * state, radio_frame_t * frame, bool high_priority);
bool state_peek_next_frame_to_transmit(state_t * state);
radio_frame_t * state_get_next_frame_to_transmit(state_t * state);
void state_abort_transceiving(state_t * state);
void state_run(state_t * state);
bool state_generate_random_ip(state_t * state);
bool state_read_ip(state_t * state, char * path);
bool state_write_ip(state_t * state, char * path, char * ip_str);
void state_ip_changed_callback(state_t * state);
bool state_read_mac(state_t * state, char * path);
bool state_write_mac(state_t * state, char * path, char * mac_str);
bool state_read_callsign(state_t * state, char * path);
bool state_write_callsign(state_t * state, char * path, char * callsign);
void state_destroy(state_t ** to_destroy);

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// uint16_ptr_t definition start///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

uint16_t * uint16_ptr(uint16_t val);
void uint16_ptr_destroy(uint16_t ** to_destroy);

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// tlv_t definition start///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

///<helper struct for unpacking type length value data structures
typedef struct __attribute__((__packed__)) tl_DEF {
	uint8_t type;
	uint16_t len;
	uint8_t pay; ///<the first byte of the payload, not the entire payload
} tl_t;

#define TLV_TYPE_LEN_SIZE 3 ///<number of bytes type and length fields take up

struct __attribute__((__packed__)) tlv_DEFINITION {
	uint8_t type;
	uint16_t len;
	uint8_t * value;
};

tlv_t * tlv(uint8_t type, uint16_t len, uint8_t * value);
int tlv_len(uint16_t len);
void tlv_destroy(tlv_t ** to_destroy);

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// frame_t definition start//////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

struct __attribute__((__packed__)) frame_header_DEFINITION {
	uint8_t callsign[8]; ///<the src callsign of the frame
	uint8_t opt1; ///<unused option byte
	uint8_t mac_tail[3]; ///<the last three bytes of the sending radio's MAC
	uint16_t len; ///<len of the payload that is used
	uint32_t crc; ///<crc32 of the remaining payload	
};

void frame_header_print(frame_header_t * hdr);

#define FRAME_LEN_MAX UNCODED_LEN

///<768 byte long decoded frame equal to UNCODED_LEN in turbo_wrapper.h
struct frame_DEFINITION {
	frame_header_t hdr;
	zlistx_t * payload; ///<a list of tlv_t
};

/** @brief create a new frame_t object that is empty
 */
frame_t * frame_empty(void);

/** @brief create a new frame_t object with callsign and options set with empty data payload
 *  @param callsign the callsign of the frame header
 *  @param opt1 options in the frame header
 *  @param mac  the mac address to use in the frame header
 */
frame_t * frame(char * callsign, uint8_t opt1, uint8_t * mac);

/** @brief append a tlv value to the frame so long as it doesn't make the frame too large
 *  @return returns true when data was added to the frame, returns false when the frame is full
 */
bool frame_append(frame_t * frame, uint8_t type, uint16_t len, uint8_t * value);
bool frame_append_tlv(frame_t * frame, tlv_t * to_append);

/** @brief get the length of the frame if it was packed, return -1 on error
 */
int frame_len(frame_t * frame);

/** @brief create a new packed_frame_t from a frame_t object, frame_t object is destroyed in the process
 */
packed_frame_t * frame_pack(frame_t ** frame);

void frame_destroy(frame_t ** to_destroy);

///<768 byte long decoded frame equal to UNCODED_LEN in turbo_wrapper.h
struct __attribute__((__packed__)) packed_frame_DEFINITION {
	frame_header_t hdr;
	uint8_t payload[750];
};

/** @brief encode a packed radio frame and return a new radio_frame_t
 *  @param to_encode double pointer to the packed_frame_t to encode, to_encode will be destroyed on success
 */
radio_frame_t * packed_frame_encode(packed_frame_t ** to_encode);
/** @brief unpack a packed frame and return the frame_t object containing its contents
 *  @param to_unpack the frame to unpack, destroyed in the process of unpacking
 *  @return returns the unpacked frame on success, returns NULL otherwise
 */
frame_t * packed_frame_unpack(packed_frame_t ** to_unpack);
void packed_frame_destroy(packed_frame_t ** to_destroy);

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// radio_frame_t definition start//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

///<class to contain an encoded radio frame that is ready to get transmitted or freshly received
struct radio_frame_t_DEFINITION {
	uint8_t * frame_ptr;
	uint8_t * frame_position; ///<current position in the frame for transmit purposes
	size_t frame_len;
};

radio_frame_t * radio_frame(size_t len);
bool radio_frame_transmit_start(radio_frame_t * frame, uint8_t channel);
bool radio_frame_receive_start(radio_frame_t * frame);
void radio_frame_reset(radio_frame_t * frame);

///<radio frame is currently being transmitted or received
#define radio_frame_active(frame) (frame->frame_position != NULL)

/** @brief calculate how many bytes are remaining to be transmitted or received for the radio frame
 *  @return returns -1 if frame is not active, returns positive value otherwise
 */
int radio_frame_bytes_remaining(radio_frame_t * rf);

/** @brief decode a radio frame to a packed_frame_t, check CRC for integrity
 *  @to_decode a pointer to the radio frame to be decoded, to_decoded will be reset
 */
packed_frame_t * radio_frame_decode(radio_frame_t * to_decode);
void radio_frame_destroy(radio_frame_t ** to_destroy);

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// peer_t definition start//////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

struct peer_t_DEFINITION {
	uint8_t callsign[8];
	uint8_t ip[4];
	uint8_t mac[6];
	int64_t last_update_ms; ///<the last time a message was received from the peer
	bool ip_valid;
	bool link; ///<true if this is a link peer
	bool slip; ///<true if a route on the slip interface has been established for this peer
};

///create a peer object from a beacon
peer_t * create_peer_from_beacon(beacon_t * beacon);

///create a peer object from a header
peer_t * create_peer_from_header(frame_header_t * hdr);

///update an existing peer object's beacon
void peer_update_beacon(peer_t * p, beacon_t * b);

///update an existing peer object's header
void peer_update_header(peer_t * p, frame_header_t * hdr);

///check if a peer object is stale, return true if it is
bool peer_check_stale(peer_t * p);

void peer_print(peer_t * p);

char * peer_callsign(peer_t * p);
char * peer_ip_str(peer_t * p);
char * peer_mac_str(peer_t * p);

bool peer_ip_valid(peer_t * p);

/* @brief open a slip port for this peer if it has a valid IP address
   @return returns false if slip port could not be opened or if the peer did not have a valid IP
*/
bool peer_open_slip(peer_t * p);

///destroy a peer object
void peer_destroy(peer_t ** to_destroy);

#endif
