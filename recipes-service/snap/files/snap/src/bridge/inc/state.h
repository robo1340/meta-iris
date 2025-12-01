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

#include "composite_encoder.h"

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

///typedef struct to contain the current state of the event_detector
struct state_t_DEFINITION {

	zlistx_t * si4463_load_order; ///<a list of si4463 config settings to load first

	//settings derives from config contents
	uint8_t tx_backoff_min_ms;
	uint8_t current_channel;

	composite_encoder_t * encoder;
	zsock_t * pub;

	//si4463 config settings
	zhashx_t * radio_config; ///<keys are defines from a radio config, values are zchunk_t containing the definition
	
	radio_frame_t * receiving;	 ///<not equal to null when a radio frame is being received	
	radio_frame_t * transmitting;///<not equal to null when a radio frame is transmitting
	
	zlistx_t * send_queue; ///<a list of radio_frame_t to be sent
	
	task_timer_t transmit_send_queue;
	csma_t csma;
	
};

state_t * state(composite_encoder_t * encoder, zsock_t * pub);

bool state_transmitting(state_t * state);
bool state_receiving(state_t * state);
bool state_transceiving(state_t * state);

/** @brief tracks when it is time to send off the data on the loading dock
	@param latched_rssi the rssi when the preamble was being detected
 *  @return returns false on failure
 */
bool state_process_receive_frame(state_t * state, zframe_t ** to_rx, uint8_t latched_rssi);
#define state_loading_dock_empty(state) (state->loading_dock.created_ms < 0)
bool state_add_frame_to_send_queue(state_t * state, uint8_t * pay, uint32_t len);
bool state_peek_next_frame_to_transmit(state_t * state);
radio_frame_t * state_get_next_frame_to_transmit(state_t * state);
void state_finish_receiving(state_t * state);
void state_abort_transceiving(state_t * state);
void state_run(state_t * state);

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

#endif
