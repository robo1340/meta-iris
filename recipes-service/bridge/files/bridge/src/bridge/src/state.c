//#include "turbo_wrapper.h"
#include "state.h"
#include "print_util.h"
#include "json_util.h"
#include "radio_events.h"
#include "beacon.h"
#include "ipv4.h"
#include "timer.h"
#include "radio_hal.h" //radio_hal_init()
#include "tlv_types.h"
#include "slip.h"
#include "serial.h"
#include "avg.h" //exp_moving_avg(uint32_t new_sample);

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <czmq.h>
#include <zlib.h>
#include <libserialport.h>

#include "radio.h"
#include "csma.h"
#include "turbo_encoder.h"

//reporting debug definitions
#define DEBUG
//#define OBJECT_DESTROY_DEBUG
//#define PACKAGE_TYPE_DEBUG
#define TX_START_DEBUG

state_t * global_state;

#define IP_ADDR_FMT_STR "10.%d.%d.254" ///<first octet shared by everyone, second and third is unique to the iris node, fourth is 1 for DHCP server, 254, for radio IP
#define MAC_0 0x02
#define MAC_1 0x00
#define MAC_2 0x00
#define MAC_ADDR_FMT_STR "02:00:00:%02x:%02x:%02x"
#define CALLSIGN_FMT_STR "iris_%u%u"

#define IP_PATH "/bridge/conf/radio_ip.json"
#define MAC_PATH "/bridge/conf/radio_mac.json"
#define CALLSIGN_PATH "/bridge/conf/radio_callsign.json"
#define CONFIG_PATH "/bridge/conf/config.json"
#define DEFAULT_CONFIG_PATH "/bridge/conf/default/config.json.default"
#define RADIO_OTHER_CONFIG_DIR "/bridge/conf/si4463/other/"			
#define RADIO_MODEM_CONFIG_DIR "/bridge/conf/si4463/modem/"											  



static bool state_read_config_settings(state_t * state, char * path);
static void state_remove_stale_peers(state_t * state);

#define MAC_COLLISION_LIMIT 3
#define IP_COLLISION_LIMIT 3

static void state_check_for_mac_collision(state_t * state, uint8_t mac_tail[3]){
	static uint8_t mac_collisions = 0; ///<the number of times this node has had a mac collision
	if (memcmp(&state->mac[3], mac_tail, 3)==0){
		printf("WARNING: header MAC collision my mac is %s\n", state->mac_str);
		if (mac_collisions < MAC_COLLISION_LIMIT){
			zsys_file_delete(MAC_PATH);
			state_read_mac(state, MAC_PATH);
		}
	}	
}

static void state_check_for_ip_collision(state_t * state, uint8_t ip[4], uint8_t mac[6]){
	static uint8_t ip_collisions = 0; ///<the number of times this node has had an IP collision
	if ((memcmp(state->ip, ip, sizeof(state->ip))==0) && (memcmp(state->mac, mac, sizeof(state->mac))!=0)){
		printf("WARNING: received beacon with IP collision, my IP is %s\n", state->ip_str);
		if (ip_collisions < IP_COLLISION_LIMIT){
			ip_collisions++;
			zsys_file_delete(IP_PATH);
			state_read_ip(state, IP_PATH);
			state_ip_changed_callback(state);
		}
	}
}

static int slip_init(state_t * state, char * ip){
	char str[128];
	system("python3 /bridge/scripts/create_slip.py -k");
	snprintf(str, sizeof(str), "python3 /bridge/scripts/create_slip.py -ip %s", ip);
	FILE * fp = popen(str, "r");
	if (fp == NULL) {return -1;}
	fgets(str, sizeof(str), fp); //ex. {"interface": "sl0", "serial_device": "/dev/ttyV1"}
	pclose(fp);
	
	char * val = json_lookup(str, sizeof(str), "serial_device");
	if (val == NULL){
		printf("WARNING: failed to open slip port for %s\n", ip);
		return -1;
	} else {
		strncpy(state->serial_path, val, sizeof(state->serial_path));
	}
	val = json_lookup(str, sizeof(str), "interface");
	if (val == NULL){
		printf("WARNING: failed to retrieve slip interface name\n");
		return -1;
	} else {
		strncpy(state->slip_if, val, sizeof(state->slip_if));
	}
	printf("INFO: slip interface \"%s\" to serial port \"%s\"\n", state->slip_if, state->serial_path);

	int to_return = serial_open(state->serial_path); //init slip port
	if (to_return < 0) {
		printf("ERROR: failed to open slip serial port %s\n", state->serial_path);
	}
	return to_return;
}

bool state_restart_wifi_ap(state_t * state){
	//char str[256];
	//snprintf(str, sizeof(str), "python3 /bridge/scripts/create_wifi_ap.py -ip 192.168.3.1 -s %s -p %s", state->wifi_ssid, state->wifi_passphrase);
	//snprintf(str, sizeof(str), "python3 /bridge/scripts/create_wifi_ap.py -ip %u.%u.%u.1 -s %s -p %s", state->ip[0], state->ip[1], state->ip[2], state->wifi_ssid, state->wifi_passphrase);
	//system(str);
	return true;
}

state_t * state(void){
	state_t * to_return = (state_t*)malloc(sizeof(state_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->si4463_load_order = zlistx_new();
	if (to_return->si4463_load_order == NULL) {return NULL;}
	zlistx_set_destructor(to_return->si4463_load_order, (zlistx_destructor_fn*)zstr_free);

	if (!state_read_ip(to_return, IP_PATH)){return NULL;}
	if (!state_read_mac(to_return, MAC_PATH)){return NULL;}
	if (!state_read_callsign(to_return, CALLSIGN_PATH)){return NULL;}
	printf("INFO \"%s\" at %s,%s\n", to_return->callsign, to_return->ip_str, to_return->mac_str);
	
	//read in settings from config.json
	state_read_config_settings(to_return, CONFIG_PATH);

	//read the radio config settings 
	to_return->radio_config = radio_load_configs(RADIO_MODEM_CONFIG_DIR, NULL, RADIO_MODEM_CONFIG_REGEX);
	to_return->radio_config = radio_load_configs(RADIO_OTHER_CONFIG_DIR, to_return->radio_config, RADIO_CONFIG_REGEX);
	if (to_return->radio_config == NULL){
		printf("CRITICAL ERROR: could not load radio config settings\n");
		return NULL;
	}
	
	if (!radio_init(to_return->radio_config, to_return->si4463_load_order)){
		printf("ERROR: radio_init() failed!\n");
	}
	
	to_return->tx_backoff_min_ms = to_return->transmit_queue_period_ms - to_return->transmit_queue_variance_ms;
	
	to_return->slip = slip_init(to_return, to_return->ip_str);
	
	////create a radio frame send queue
	to_return->send_queue = zlistx_new();
	if (to_return->send_queue == NULL) {return NULL;}
	zlistx_set_destructor(to_return->send_queue, (zlistx_destructor_fn*)radio_frame_destroy);
	
	to_return->loading_dock.created_ms = -1;
	to_return->loading_dock.frame = NULL;
	to_return->loading_dock.max_idle_ms = to_return->max_loading_dock_idle_ms;
	
	to_return->transmitting = NULL;
	to_return->receiving	= radio_frame(sizeof(coded_block_t));
	
	timer_init(&to_return->beacon_transmit, 	true, 	to_return->beacon_period_ms, to_return->beacon_period_variance_ms);
	timer_init(&to_return->transmit_send_queue, true,   to_return->transmit_queue_period_ms, to_return->transmit_queue_variance_ms);
	timer_charge(&to_return->beacon_transmit, RANDINT(4000,6000)); //charge up the first beacon to happen within a specified amount of time
	csma_init(&to_return->csma, 50, 5, 10); //probabillity to transmit, min backoff ms, max backoff ms
	
	
	to_return->peers			= zlistx_new();
	if (to_return->peers == NULL) {return NULL;}
	zlistx_set_destructor(to_return->peers, (zlistx_destructor_fn*)peer_destroy);
	
	to_return->link_peers		= zlistx_new();
	if (to_return->link_peers == NULL) {return NULL;}
	
	to_return->peers_by_ip		= zhashx_new();
	if (to_return->peers_by_ip == NULL) {return NULL;}
	zhashx_set_key_destructor(to_return->peers_by_ip, (zhashx_destructor_fn*) zstr_free);
	zhashx_set_key_duplicator(to_return->peers_by_ip, (zhashx_duplicator_fn*) strdup);
	
	to_return->peers_by_mac		= zhashx_new();
	if (to_return->peers_by_mac == NULL) {return NULL;}
	zhashx_set_key_destructor(to_return->peers_by_mac, (zhashx_destructor_fn*) zstr_free);
	zhashx_set_key_duplicator(to_return->peers_by_mac, (zhashx_duplicator_fn*) strdup);
	
	to_return->peers_by_callsign= zhashx_new();
	if (to_return->peers_by_callsign == NULL) {return NULL;}
	zhashx_set_key_destructor(to_return->peers_by_callsign, (zhashx_destructor_fn*) zstr_free);
	zhashx_set_key_duplicator(to_return->peers_by_callsign, (zhashx_duplicator_fn*) strdup);
	
	to_return->low_priority_queue = zlistx_new();
	if (to_return->low_priority_queue == NULL) {return NULL;}
	zlistx_set_destructor(to_return->low_priority_queue, (zlistx_destructor_fn*)zchunk_destroy);
	
	to_return->encoder = turbo_encoder(0);
	
	global_state = to_return; //set the global state object, this is ok since this is a singleton class
	
	return to_return;
}

bool state_process_receive_frame(state_t * state, frame_t ** received_frame){
	static uint8_t slip_tx_buf[1500];
	static uint8_t buf[1500]; //buffer for decompressed packets, too large at the moment
	static uint32_t len;
	static int slip_tx_len;
	
	//process the header first
	state_check_for_mac_collision(state, (*received_frame)->hdr.mac_tail);
	if (!state_update_peers_new_header(state, &(*received_frame)->hdr)) {
		printf("WARNING: state_update_peers_new_header() failed!\n");
	}
	
	tlv_t * tlv = zlistx_first((*received_frame)->payload);
	while (tlv != NULL){
		switch(tlv->type){
			case TYPE_BEACON:
				if (tlv->len != sizeof(beacon_t)){
					printf("WARNING: beacon received with incorrect length\n");
					break;
				}
				if (memcmp(state->mac, ((beacon_t*)tlv->value)->mac, sizeof(state->mac))==0){ //this is my own beacon being re-broadcast
					break;
				}
				if(!handle_beacon(state, (beacon_t*)tlv->value)){
					printf("WARNING: handle_beacon() failed!\n");
					break;
				}
				if (!state_update_peers_new_beacon(state, (beacon_t*)tlv->value)){
					printf("WARNING: state_update_peers(beacon) failed!\n");
					break;
				}
				if (beacon_ttl_decrement((beacon_t*)tlv->value)){ //re-transmit the beacon
					//if (!state_append_loading_dock(state, tlv->type, tlv->len, tlv->value)) {
					//	printf("WARNING: state_append_loading_dock() failed!\n");
					//	break;
					//}
				}
				break;
			case TYPE_IPV4:
				printf("->");
				ipv4_print((ipv4_hdr_t*)tlv->value);
				if (ipv4_check_dst(state, (ipv4_hdr_t*)tlv->value)){ //this packet was addressed to my subnet
					//printArrHex(tlv->value, tlv->len);
					//encode to slip send over virtual serial terminal
					slip_tx_len = slip_encode(tlv->value, tlv->len, slip_tx_buf, sizeof(slip_tx_buf));
					if (slip_tx_len < 0){
						printf("ERROR: slip_encode() failed!\n");
						break;
					}
					if (state->slip < 0){
						printf("ERROR: slip port not initialized!\n");
						break;
					}
					if (serial_write(state->slip, slip_tx_buf, slip_tx_len) < 0){
						printf("ERROR: sp_nonblocking_write() failed!\n");
						break;
					}
				}
				else if (ipv4_ttl_decrement(state, (ipv4_hdr_t*)tlv->value)){ //re-transmit this packet
					//if (!state_append_loading_dock(state, tlv->type, tlv->len, tlv->value)) {
					//	printf("WARNING: state_append_loading_dock() failed!\n");
					//	break;
					//}
				}
				break;
			case TYPE_IPV4_COMPRESSED:
				len = sizeof(buf);
				if (uncompress(buf, (uLongf*)&len, tlv->value, tlv->len) != Z_OK){
					printf("WARNING: failed to decompress IPV4 packed\n");
					break;
				}
				printf("-->"); ipv4_print((ipv4_hdr_t*)buf);
				if (ipv4_check_dst(state, (ipv4_hdr_t*)buf)){ //this packet was addressed to my subnet
					//printArrHex(buf, len);
					//encode to slip send over virtual serial terminal
					slip_tx_len = slip_encode(buf, len, slip_tx_buf, sizeof(slip_tx_buf));
					if (slip_tx_len < 0){
						printf("ERROR: slip_encode() failed!\n");
						break;
					}
					if (state->slip < 0){
						printf("ERROR: slip port not initialized!\n");
						break;
					}
					if (serial_write(state->slip, slip_tx_buf, slip_tx_len) < 0){
						printf("ERROR: sp_nonblocking_write() failed!\n");
						break;
					}
				}
				else if (ipv4_ttl_decrement(state, (ipv4_hdr_t*)buf)){ //re-transmit this packet
					//if (!state_append_loading_dock(state, tlv->type, tlv->len, tlv->value)) {
					//	printf("WARNING: state_append_loading_dock() failed!\n");
					//	break;
					//}
				}
				break;
			default:
				printf("WARNING: unknown type 0x%02x\n", tlv->type);
		}
		tlv = zlistx_next((*received_frame)->payload);
	}
	
	frame_destroy(received_frame); //destroy the frame now that it has been processed
	return true;
}

bool state_update_peers_new_header(state_t * state, frame_header_t * hdr){
	peer_t * p;
	
	if (memcmp(&state->mac[3], hdr->mac_tail, sizeof(hdr->mac_tail))==0){
		printf("WARNING: peer detected with matching MAC tail ");
		printArrHex(hdr->mac_tail, sizeof(hdr->mac_tail));
	}
	
	if (hdr != NULL){ //hdr argument is valid
		p = zlistx_first(state->peers);
		while (p != NULL){
			if (memcmp(&p->mac[3], hdr->mac_tail, sizeof(hdr->mac_tail))==0){ //this is a match
				peer_update_header(p, hdr);
				return true; //existing peer was found so we can return
			}
			p = zlistx_next(state->peers);
		}
		//new peer discovered
		p = create_peer_from_header(hdr);
		if (p==NULL){return false;}
		printf("INFO: New Link Peer Discovered:");
		peer_print(p);
		p->link = true;
		zlistx_add_end(state->peers, p);
		zlistx_add_end(state->link_peers, p);
		zhashx_insert(state->peers_by_mac, peer_mac_str(p), p);
	}
	return true;
}

bool state_update_peers_new_beacon(state_t * state, beacon_t * b){
	char cs[10]; //callsign null terminated
	char csb[10]; //callsign null terminated
	char ip[16];  //ip string buffer
	peer_t * p;
	
	//check for an IP collision
	state_check_for_ip_collision(state, b->ip, b->mac);
	
	if (b != NULL){ //b argument is valid
		p = zlistx_first(state->peers);
		while (p != NULL){
			if (memcmp(p->mac, b->mac, sizeof(p->mac))==0){ //this is a match
				snprintf(cs, sizeof(cs), "%.*s", sizeof(p->callsign), p->callsign);
				snprintf(csb,sizeof(csb),"%.*s", sizeof(b->callsign), b->callsign);
				if (strcmp(cs,"")==0){ //callsign has not been set yet, set it now
					zhashx_insert(state->peers_by_callsign, csb, p);
				}
				else if (strncmp(cs,csb,sizeof(cs))!=0){ //callsign changed, change hash table lookup
					zhashx_rename(state->peers_by_callsign, cs, csb);
				}
				
				snprintf(ip, sizeof(ip), "%u.%u.%u.%u", b->ip[0], b->ip[1], b->ip[2], b->ip[3]);
				if (peer_ip_valid(p)){ //ip_str not set yet
					zhashx_insert(state->peers_by_ip, ip, p);
				}
				else if (strncmp(peer_ip_str(p), ip, sizeof(ip))!=0){ //IP changed, change hash table lookup
					zhashx_rename(state->peers_by_ip, peer_ip_str(p), ip);
				}
				peer_update_beacon(p, b);
				return true; //peer was found so we can return
			}
			p = zlistx_next(state->peers);
		}
		//new Peer Found
		p = create_peer_from_beacon(b);
		if (p==NULL) {return false;}
		printf("INFO: New Peer Discovered:");
		peer_print(p);
		zlistx_add_end(state->peers, p);
		snprintf(cs, sizeof(cs), "%.*s", sizeof(p->callsign), p->callsign);
		zhashx_insert(state->peers_by_callsign, cs, p);
		zhashx_insert(state->peers_by_ip, peer_ip_str(p), p);
	}
	return true;
	
}

bool state_transmitting(state_t * state) {return (state->transmitting != NULL);}

bool state_receiving(state_t * state){
	return radio_frame_active(state->receiving);
}

bool state_transceiving(state_t * state){
	return (radio_frame_active(state->receiving) || (state->transmitting != NULL));
}

static bool add_low_priority_packet(state_t * state){
	tlv_t * p = zlistx_first(state->low_priority_queue);
	if (p == NULL){return false;} //nothing in the queue
	
	if (!frame_append_tlv(state->loading_dock.frame, p)){ //unable to append anymore
		return false;
	} else {//otherwise packet was appended
		zlistx_detach_cur(state->low_priority_queue);
		//zlistx_detach (state->low_priority_queue, p);
		return true;
	}
}

static void add_low_priority_packets(state_t * state){
	while(add_low_priority_packet(state)){
		printf("added low priority packet\n");
	}	
}

bool state_loading_dock_run(state_t * state){
	//printf("DEBUG: state_loading_dock_run()\n");
	if (state_loading_dock_empty(state)){return true;} //loading dock is empty, nothing to do
	if ((zclock_mono() - state->loading_dock.created_ms) > state->loading_dock.max_idle_ms){
		state->loading_dock.created_ms = -1; //set loading dock as empty
		
		add_low_priority_packets(state);
		packed_frame_t * pf = frame_pack(&state->loading_dock.frame);		//1. pack the frame
		if (pf == NULL) {return false;} 									////	failure to pack frame
		radio_frame_t * rf = packed_frame_encode(&pf);						//2. encode the frame
		if (rf == NULL) {return false;} 									////	failure to encode frame
		if (!state_add_frame_to_send_queue(state, rf, false)){return false;}//3. add radio frame to send queue
	}
	return true;
}

#define DEFAULT_FRAME_OPT1 0

static bool state_create_loading_dock(state_t * state){
	state->loading_dock.frame = frame(state->callsign, DEFAULT_FRAME_OPT1, state->mac);	//4. create a new loading dock frame
	if (state->loading_dock.frame == NULL){return false;}
	state->loading_dock.created_ms = zclock_mono();
	return true;
}

bool state_append_loading_dock(state_t * state, uint8_t type, uint16_t len, uint8_t * value){
	//printf("DEBUG: state_append_loading_dock()\n");
	
	if (state_loading_dock_empty(state)){
		if (!state_create_loading_dock(state)) {return false;}
	}

	if(!frame_append(state->loading_dock.frame, type, len, value)){ //the frame is full
		packed_frame_t * pf = frame_pack(&state->loading_dock.frame);		//1. pack the frame
		if (pf == NULL) {return false;} 									////	failure to pack frame
		radio_frame_t * rf = packed_frame_encode(&pf);						//2. encode the frame
		if (rf == NULL) {return false;} 									////	failure to encode frame
		if (!state_add_frame_to_send_queue(state, rf, false)){return false;}//3. add radio frame to send queue
		
		if (!state_create_loading_dock(state)) {return false;}				//4. create the new loading dock
		if(!frame_append(state->loading_dock.frame, type, len, value)){return false;} //failure to add frame to loading dock
	} else { //successfully appended to loading dock, wait for more packets
		state->loading_dock.created_ms = zclock_mono();
	}
	return true;
}

bool state_append_low_priority_packet(state_t * state, uint8_t type, uint16_t len, uint8_t * value){
	
	tlv_t * to_append = tlv(type, len, value);
	if (to_append == NULL) {
		printf("ERROR: tlv() failed\n");
		return false;
	}
	
	if(!zlistx_add_end(state->low_priority_queue, to_append)){
		printf("state_append_low_priority_packet() failed\n");
		return false;
	}
	
	return true;
}

bool state_add_frame_to_send_queue(state_t * state, radio_frame_t * frame, bool high_priority){
	//printf("DEBUG: state_add_frame_to_send_queue()\n");
	if (!high_priority){
		zlistx_add_end(state->send_queue, frame);
	} else {
		zlistx_add_start(state->send_queue, frame);
	}
	return true;
}

bool state_peek_next_frame_to_transmit(state_t * state){
	return (zlistx_size(state->send_queue) > 0);
}

radio_frame_t * state_get_next_frame_to_transmit(state_t * state){
	//printf("DEBUG: state_get_next_frame_to_transmit()\n");
	radio_frame_t * to_return = zlistx_first(state->send_queue);
	state->transmitting = to_return;
	if (to_return != NULL){
		zlistx_detach_cur(state->send_queue);
	}
	return to_return;
}

void state_abort_transceiving(state_t * state){
	printf("INFO: state_abort_transceiving()\n");
	
	radio_frame_reset(state->receiving);
	if (state->transmitting != NULL){
		radio_frame_destroy(&state->transmitting);
	}
	radio_start_rx(state->current_channel, sizeof(coded_block_t));
	//si446x_change_state(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY); //change to ready state
}

#define TXRX_TIMEOUT_MS 4000 ///<timeout value used to detect when the program is in a locked state

#define RSSI_THRESHHOLD 10 ///<only transmit if AVG RSSI plus this is lower than this

static void state_tx_now(state_t * state){
	//printf("INFO: state_tx_now\n");
	state->transmitting = state_get_next_frame_to_transmit(state);
	if (state->transmitting != NULL){
		if (!radio_frame_transmit_start(state->transmitting, state->current_channel)){
			printf("ERROR: radio_frame_transmit_start() failed!\n");
		}
	} else {
		printf("ERROR: state_get_next_frame_to_transmit() returned NULL\n");
	}
}

void state_run(state_t * state){
	static int64_t txrx_timer = -1;
	static task_timer_t check_for_stale_peers = {true, 30000, 0, 0};
	//static task_timer_t radio_state_check = {false, 250, 0, 0};
	//static task_timer_t radio_rssi_check  = {false, 5, 0};
	static task_timer_t radio_peers_print = {true, 60000, 0, 0};
	static beacon_t my_beacon;
	static uint32_t rssi, avg_rssi;

	if (state_transceiving(state)) {
		radio_event_callback(state);
		if (txrx_timer < 0){
			txrx_timer = zclock_mono();
		}
		else if ((zclock_mono()-txrx_timer)>TXRX_TIMEOUT_MS){
			printf("timed out ");
			state_abort_transceiving(state);
			txrx_timer = -1;
		}
		return; //return immediately, when transmitting or receiving focus only on that
	} else if (txrx_timer > 0) {
		txrx_timer = -1;
	}
	
	//if (timer_run(&radio_rssi_check)){
	//	radio_print_modem_status(false);
	//}
	
	if (!state_loading_dock_run(state)){
		printf("ERROR: state_loading_dock_run() failed!\n");
	}
	
	avg_rssi = exp_moving_avg(radio_get_rssi());
	
	if (csma_enabled(&state->csma)) { //ready to transmit but medium is busy, check it after random backoff then probabalitistically transmit
		if (csma_run_countdown(&state->csma)){
			rssi = radio_get_rssi();
			printf("waiting rssi %u\n", rssi);
			if (rssi < (avg_rssi+RSSI_THRESHHOLD)){
				if (csma_run_transmit(&state->csma)){ //transmit, csma will be disabled now
					state_tx_now(state);
				}			
			}
		}
	}
	else if (timer_run(&state->transmit_send_queue)) { //csma is not running start a transmission
		
		if (!state_peek_next_frame_to_transmit(state)){ //if there is nothing to transmit set back-off to its minimum
			state->transmit_send_queue.period_ms = state->tx_backoff_min_ms;
		} else { //there is a frame to transmit
			rssi = radio_get_rssi();
			printf("rssi %u\n", rssi);
			if (rssi < (avg_rssi+RSSI_THRESHHOLD)){
				state_tx_now(state);
			} else {
				csma_reset(&state->csma); //start the csma algorithm
			}			
		}
	}
	
	//csma_enabled(csma)
	//void csma_reset(csma_t * csma);
	//void csma_disable(csma_t * csma);
	//bool csma_run_countdown(csma_t * csma);
	//bool csma_run_transmit(csma_t * csma);

	if (timer_run(&state->beacon_transmit)){ //add a beacon to the transmit queue
		printf("INFO: beacon transmit\n");
		beacon_init(&my_beacon, state);
		if (!state_append_loading_dock(state, TYPE_BEACON, sizeof(beacon_t), (uint8_t*)&my_beacon)){
			printf("ERROR: state_append_loading_dock() failed!\n");
		}
	}
	
	//if (timer_run(&radio_state_check)){
	//	radio_print_state(false);
	//}
	
	if (timer_run(&radio_peers_print)){
		uint32_t num_peers = zlistx_size(state->peers);
		if (num_peers > 0) {
			printf("\nPeers: %u\n", num_peers);
			peer_t * p = zlistx_first(state->peers);
			while (p != NULL){
				printf("\t");
				peer_print(p);
				p = zlistx_next(state->peers);
			}
			printf("\n");
		}
	}
	
	if (timer_run(&check_for_stale_peers)){
		state_remove_stale_peers(state);
	}
}


static void state_remove_stale_peers(state_t * state){
	zlistx_t * stale = NULL;
	peer_t * p = zlistx_first(state->peers);
	while (p != NULL){
		if (peer_check_stale(p)){
			if (stale == NULL){
				stale = zlistx_new();
				if (state == NULL) {return;}
			}
			zlistx_add_end(stale, p);
		}
		p = zlistx_next(state->peers);
	}
	if (stale == NULL) {return;} //no stale peers
	
	p = zlistx_first(stale);
	peer_t * d;
	while (p != NULL){
		zhashx_delete(state->peers_by_ip, peer_ip_str(p));
		zhashx_delete(state->peers_by_mac, peer_mac_str(p));
		zhashx_delete(state->peers_by_callsign, peer_callsign(p));
		d = zlistx_find(state->peers, p);
		if (d != NULL){
			zlistx_detach(state->peers, d);
		}
		
		d = zlistx_find(state->link_peers, p);
		if (d != NULL){
			zlistx_detach(state->link_peers, d);
		}
		peer_destroy(&p);
		//might need to remove the peer's old route eventually
		p = zlistx_next(state->peers);
	}
	zlistx_destroy(&stale);
}

bool state_generate_random_ip(state_t * state){
	printf("INFO: state_generate_random_ip()\n");
	snprintf(state->ip_str, sizeof(state->ip_str), IP_ADDR_FMT_STR, RANDINT(1,254), RANDINT(1,254));
	if (!state_write_ip(state, IP_PATH, state->ip_str)){
		printf("ERROR: failed to write to \'%s\'\n", IP_PATH);
		return false;
	}
	int rc = sscanf(state->ip_str, "%hhu.%hhu.%hhu.%hhu", &state->ip[0], &state->ip[1], &state->ip[2], &state->ip[3]);
	return (rc == 4);
}

bool state_read_ip(state_t * state, char * path){
	if (!read_json_str(path, state->ip_str, sizeof(state->ip_str))){
		printf("WARNING: failed to read IP from \'%s\', generating new one\n", path);
		return state_generate_random_ip(state);
	}
	int rc = sscanf(state->ip_str, "%hhu.%hhu.%hhu.%hhu", &state->ip[0], &state->ip[1], &state->ip[2], &state->ip[3]);
	return (rc == 4);
}

bool state_write_ip(state_t * state, char * path, char * ip_str){
	printf("state_write_ip(%s) -> \'%s\'\n", ip_str, path);
	return write_json_str(path, ip_str);
}

void state_ip_changed_callback(state_t * state){
	printf("INFO: state_ip_changed_callback()\n");
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "python3 /bridge/scripts/create_slip.py -ip %s", state->ip_str);
	system(cmd);
	
	//if (!state_restart_wifi_ap(state)){ // reconfigure wifi AP here
	//	printf("WARNING: failed to start wifi AP!\n");
	//}
}

bool state_read_mac(state_t * state, char * path){
	if (!read_json_str(path, state->mac_str, sizeof(state->mac_str))){
		printf("WARNING: failed to read MAC from \'%s\', generating new one\n", path);
		snprintf(state->mac_str, sizeof(state->mac_str), MAC_ADDR_FMT_STR, (uint8_t)RANDINT(0,255), (uint8_t)RANDINT(0,255), (uint8_t)RANDINT(0,255));
		if (!state_write_mac(state, path, state->mac_str)){
			printf("ERROR: failed to write to \'%s\'\n", path);
			return false;
		}
	}
	int rc = sscanf(state->mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &state->mac[0], &state->mac[1], &state->mac[2], &state->mac[3], &state->mac[4], &state->mac[5]);
	return (rc == 6);
}

bool state_write_mac(state_t * state, char * path, char * mac_str){
	printf("state_write_mac(%s) -> \'%s\'\n", mac_str, path);
	return write_json_str(path, mac_str);
}

bool state_read_callsign(state_t * state, char * path){
	if (!read_json_str(path, state->callsign, sizeof(state->callsign))){
		printf("WARNING: failed to read callsign from \'%s\', generating new one\n", state->callsign);
		snprintf(state->callsign, sizeof(state->callsign), CALLSIGN_FMT_STR, RANDINT(0,255), RANDINT(0,255));
		if (!state_write_callsign(state, path, state->callsign)){
			printf("ERROR: failed to write to \'%s\'\n", path);
			return false;
		}
	}
	return true;
}

bool state_write_callsign(state_t * state, char * path, char * callsign){
	printf("state_write_callsign(%s) -> \'%s\'\n", callsign, path);
	return write_json_str(path, callsign);
}

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
}
*/
static bool state_read_config_settings(state_t * state, char * path){
	char * val;
	bool read_failed = false;
	int num_toks;
	uint32_t fsize;
	void * tokens = NULL;
	
	char json_str[1500];
	FILE * f = fopen(path,"rb");
	if (f==NULL){
		printf("ERROR, failed to load config file at \"%s\", using default settings\n", path);
		read_failed = true;
	} else {
		fseek(f, 0, SEEK_END);
		fsize = ftell(f);
		if (fsize > sizeof(json_str)){
			printf("ERROR, config file at \"%s\" is to large to fit in buffer, using default settings\n", path);
			read_failed = true;
		}
		fseek(f, 0, SEEK_SET);  // same as rewind(f); 
		fread(json_str, fsize, 1, f);
		fclose(f);
		json_str[fsize] = '\0';
	}
	
	if (!read_failed){
		tokens = json_load(json_str, fsize, &num_toks);
		if (tokens == NULL){
			printf("WARNING: failed to parse contents of config at \"%s\" using default settings\n", path);
			read_failed = true;		
		}
	}
	val = jsmn_json_lookup(tokens, num_toks, json_str, "ttl_set");
	state->ttl_set   					 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_TTL_SET;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "ttl_drop");
	state->ttl_drop  					 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_TTL_DROP;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "beacon_period_ms");
	state->beacon_period_ms			 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_BEACON_PERIOD_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "beacon_period_variance_ms");
	state->beacon_period_variance_ms	 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_BEACON_PERIOD_VARIANCE_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "beacon_ttl");
	state->beacon_ttl					 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_BEACON_TTL;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "transmit_queue_period_ms");
	state->transmit_queue_period_ms	 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_TRANSMIT_QUEUE_PERIOD_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "transmit_queue_variance_ms");
	state->transmit_queue_variance_ms	 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_TRANSMIT_QUEUE_VARIANCE_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "link_peer_period_ms");
	state->link_peer_period_ms			 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_LINK_PEER_PERIOD_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "link_peer_variance_ms");
	state->link_peer_variance_ms		 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_LINK_PEER_VARIANCE_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "link_peer_ttl");
	state->link_peer_ttl				 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_LINK_PEER_TTL;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "link_peer_timeout_ms");
	state->link_peer_timeout_ms		 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_LINK_PEER_TIMEOUT_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "user_period_ms");
	state->user_period_ms				 	= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_USER_PERIOD_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "user_variance_ms");
	state->user_variance_ms			 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_USER_VARIANCE_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "user_ttl");
	state->user_ttl					 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_USER_TTL;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "niceness");
	state->niceness					 		= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_NICENESS;
	if (state->niceness != 0) {
		printf("INFO: setting niceness to %d\n", state->niceness);
		setpriority(PRIO_PROCESS, 0, state->niceness);
	}
	val = jsmn_json_lookup(tokens, num_toks, json_str, "max_loading_dock_idle_ms");
	state->max_loading_dock_idle_ms			= (val != NULL) ? strtol(val, NULL, 10) : DEFAULT_MAX_LOADING_DOCK_IDLE_MS;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "radio_default_channel");
	state->current_channel					= (val != NULL) ? strtol(val, NULL, 10) : RADIO_DEFAULT_CHANNEL;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "compress_ipv4");
	state->compress_ipv4					= (val != NULL) ? (memcmp(val,"true",4)==0) : DEFAULT_COMPRESS_IPV4;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "slip_mtu");
	state->slip_mtu							= (val != NULL) ? strtol(val, NULL, 10) : false;
	val = jsmn_json_lookup(tokens, num_toks, json_str, "wifi_ssid");
	if (val == NULL){
		strncpy(state->wifi_ssid, DEFAULT_WIFI_SSID, sizeof(state->wifi_ssid));
	} else if (memcmp(val,"{callsign}",10)==0) {
		strncpy(state->wifi_ssid, "callsign_", sizeof(state->wifi_ssid));
		strncat(state->wifi_ssid, state->callsign, strlen(state->wifi_ssid)+sizeof(state->callsign));
	} else {
		strncpy(state->wifi_ssid, val, sizeof(state->wifi_ssid));
	}
	printf("wifi SSID: \"%s\"\n", state->wifi_ssid);
	val = jsmn_json_lookup(tokens, num_toks, json_str, "wifi_passphrase");
	strncpy(state->wifi_passphrase, ((val != NULL) ? val : DEFAULT_WIFI_PASSPHRASE), sizeof(state->wifi_passphrase));
	printf("INFO: wifi ssid \"%s\", passphrase \"%s\"\n", state->wifi_ssid, state->wifi_passphrase);
	
	if (!jsmn_json_load_array(tokens, num_toks, json_str, "si4463_load_order", state->si4463_load_order)){ //load the defaults
		zlistx_add_end(state->si4463_load_order, "RF_POWER_UP");
	}
	val = jsmn_json_lookup(tokens, num_toks, json_str, "low_priority_udp_port");
	state->low_priority_udp_port			= (val != NULL) ? strtol(val, NULL, 10) : LOW_PRIORITY_UDP_PORT;
	
	if (read_failed == true){
		char cmd[256];
		snprintf(cmd, sizeof(cmd), "cp %s %s", DEFAULT_CONFIG_PATH, CONFIG_PATH);
		system(cmd);
	}
	
	return true;
}

void state_destroy(state_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying state_t at 0x%p\n", (void*)*to_destroy);
#endif
	system("python3 /bridge/scripts/create_slip.py -k");
	radio_hal_deinit();
	zlistx_destroy(&(*to_destroy)->send_queue);
	
	if ((*to_destroy)->slip > 0){
		serial_close((*to_destroy)->slip);
	}
	
	zlistx_destroy(&(*to_destroy)->si4463_load_order);
	zlistx_destroy(&(*to_destroy)->peers);
	zlistx_destroy(&(*to_destroy)->link_peers);
	zhashx_destroy(&(*to_destroy)->peers_by_ip);
	zhashx_destroy(&(*to_destroy)->peers_by_mac);
	zhashx_destroy(&(*to_destroy)->peers_by_callsign);
	zhashx_destroy(&(*to_destroy)->radio_config);	
	zlistx_destroy(&(*to_destroy)->low_priority_queue);
	
	turbo_encoder_destroy(&(*to_destroy)->encoder);
	
	free(*to_destroy); //free the state_t from memory
	*to_destroy = NULL; //set the state_t* to be a NULL pointer
	return;	
}

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// uint16_ptr_t definition start///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

uint16_t * uint16_ptr(uint16_t val){
	uint16_t * to_return = (uint16_t*)malloc(sizeof(uint16_t));
	*to_return = val;
	return to_return;
}

void uint16_ptr_destroy(uint16_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying uint16_t at 0x%p\n", (void*)*to_destroy);
#endif
	free(*to_destroy);
	*to_destroy = NULL;
	return;		
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// tlv_t definition start///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

tlv_t * tlv(uint8_t type, uint16_t len, uint8_t * value){
	tlv_t * to_return = (tlv_t*)malloc(sizeof(tlv_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->type = type;
	to_return->len  = len;
	
	to_return->value = (uint8_t*)malloc(len);
	if (to_return->value == NULL) {
		free(to_return);
		return NULL;
	}
	if (value != NULL){
		memcpy(to_return->value, value, len);
	}
	return to_return;
}

int tlv_len(uint16_t len){
	return (sizeof(uint8_t) + sizeof(uint16_t) + len);
}

void tlv_destroy(tlv_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying tlv_t at 0x%p\n", (void*)*to_destroy);
#endif
	if ((*to_destroy)->value != NULL){
		free((*to_destroy)->value);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// frame_t definition start//////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void frame_header_print(frame_header_t * hdr){
	printf("HEADER: %.*s | ::%02x:%02x:%02x | %u 0x%08x\n", sizeof(hdr->callsign), hdr->callsign, 
		hdr->mac_tail[0], hdr->mac_tail[1], hdr->mac_tail[2], hdr->len, hdr->crc
	);
}


frame_t * frame_empty(void){
	frame_t * to_return = (frame_t*)malloc(sizeof(frame_t));
	if (to_return == NULL) {return NULL;}
	
	//create a payload list
	to_return->payload = zlistx_new();
	if (to_return->payload == NULL) {return NULL;}
	zlistx_set_destructor(to_return->payload, (zlistx_destructor_fn*)tlv_destroy);
	
	return to_return;	
}

frame_t * frame(char * callsign, uint8_t opt1, uint8_t * mac){
	frame_t * to_return = frame_empty();
	if (to_return == NULL) {return NULL;}
	
	memset(&to_return->hdr, 0, sizeof(to_return->hdr));
	memcpy(to_return->hdr.callsign, callsign, sizeof(to_return->hdr.callsign));
	to_return->hdr.opt1 = opt1;
	memcpy(to_return->hdr.mac_tail, &mac[3], 3);
	return to_return;
}

bool frame_append(frame_t * frame, uint8_t type, uint16_t len, uint8_t * value){
	if ((frame_len(frame) + tlv_len(len)) > FRAME_LEN_MAX){ //cannot append value, frame would be too large
		return false;
	}
	
	tlv_t * to_append = tlv(type, len, value);
	if (to_append == NULL) {
		printf("ERROR: tlv() failed\n");
		return false;
	}
	
	if (zlistx_add_end(frame->payload, to_append) == NULL){
		printf("ERROR: zlistx_add_end() failed\n");
		return false;
	}	
	return true;
}

bool frame_append_tlv(frame_t * frame, tlv_t * to_append){
	if ((frame_len(frame) + to_append->len) > FRAME_LEN_MAX){ //cannot append value, frame would be too large
		return false;
	}
	
	if (zlistx_add_end(frame->payload, to_append) == NULL){
		printf("ERROR: zlistx_add_end() failed\n");
		return false;
	}	
	return true;	
}

int frame_len(frame_t * frame){
	int rc = sizeof(frame->hdr);
	tlv_t * p = zlistx_first(frame->payload);
	while (p != NULL){
		rc += TLV_TYPE_LEN_SIZE + p->len;
		p = zlistx_next(frame->payload);
	}
	return rc;
}

#define DEBUG_FRAME_LOAD_RATIO

packed_frame_t * frame_pack(frame_t ** frame){
	int len = frame_len(*frame);
	if (len > (int)sizeof(packed_frame_t)){
		printf("ERROR: frame_pack() length check failed\n");
		return NULL;
	}
#ifdef DEBUG_FRAME_LOAD_RATIO
	float flen = len;
	float fsz = sizeof(packed_frame_t);
	printf("DEBUG: frame_pack %d/%u %4.1f%%\n", len, sizeof(packed_frame_t), (flen/fsz)*100);
#endif
	
	packed_frame_t * to_return = (packed_frame_t*)malloc(sizeof(packed_frame_t));
	if (to_return == NULL) {return NULL;}
	
	memcpy(&to_return->hdr, &(*frame)->hdr, sizeof(to_return->hdr)); //copy header directly
	memset(to_return->payload, 0, sizeof(to_return->payload));
	
	tlv_t * p = zlistx_first((*frame)->payload);
	uint8_t * ptr = to_return->payload;
	while (p != NULL){
		memcpy(ptr, p, TLV_TYPE_LEN_SIZE);
		ptr += TLV_TYPE_LEN_SIZE;
		memcpy(ptr, p->value, p->len);
		ptr += p->len;
		p = zlistx_next((*frame)->payload);
	}
	
	frame_destroy(frame);
	return to_return;
}

void frame_destroy(frame_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying frame_t at 0x%p\n", (void*)*to_destroy);
#endif
	zlistx_destroy(&(*to_destroy)->payload);
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// packed_frame_t definition start///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

static void packed_frame_generate_crc(packed_frame_t * frame){
	uint32_t crc = crc32(0, NULL, 0);
	frame->hdr.crc = crc32(crc, frame->payload, sizeof(frame->payload));
	return;
}

radio_frame_t * packed_frame_encode(packed_frame_t ** to_encode){
	//printf("DEBUG: packed_frame_encode()\n");
	radio_frame_t * to_return = radio_frame(sizeof(coded_block_t));
	if (to_return == NULL) {return NULL;}
	
	(*to_encode)->hdr.len = sizeof((*to_encode)->payload);
	packed_frame_generate_crc(*to_encode);//calculate the CRC
	
	//frame_header_print(&(*to_encode)->hdr);

	uint8_t * encoded = turbo_encoder_encode(global_state->encoder, (uint8_t*)(*to_encode), sizeof(packed_frame_t));
	if (encoded == NULL){
	//if(!turbo_wrapper_encode((uncoded_block_t*), (coded_block_t*)to_return->frame_ptr)){
		radio_frame_destroy(&to_return);
		return NULL;
	}
	memcpy(to_return->frame_ptr, encoded, sizeof(coded_block_t));
	
	packed_frame_destroy(to_encode);
	return to_return;
}

frame_t * packed_frame_unpack(packed_frame_t ** to_unpack){
	frame_t * to_return = frame_empty();
	if (to_return == NULL) {return NULL;}
	
	memcpy(&to_return->hdr, &(*to_unpack)->hdr, sizeof(to_return->hdr)); //copy the header over byte for byte
	//placeholder for where header options would be inspected
	
	tl_t * tl= (tl_t*)((*to_unpack)->payload); //point to the first byte of the unpacked payload, should align with tl_t
	uint8_t * payload_end = &(*to_unpack)->payload[sizeof((*to_unpack)->payload)-1];
	uint32_t tlv_size;
	int rc = 0; //the number of tlv subframes pulled from the payload
	
	//printArrHex((*to_unpack)->payload, sizeof((*to_unpack)->payload));
	
	while(rc >= 0){
#ifdef PACKAGE_TYPE_DEBUG
		printf("DEBUG: type 0x%x, len: 0x%04x\n", tl->type, tl->len);
#endif
		if (tl->type == TYPE_END) {break;} //break if the end type is encountered
		//else if (!TYPE_KNOWN(tl->type)) {break;} //unknown type
		tlv_size = TLV_TYPE_LEN_SIZE + tl->len;
		if (((uint8_t*)tl + tlv_size) > payload_end){
			printf("WARNING: continued unpacking of type 0x%02x:%u would cause overflow\n", tl->type, tlv_size);
			break;
		}
		else if (!frame_append(to_return, tl->type, tl->len, &tl->pay)){
			rc = -1; //error condition
		}
		rc++; //one more subframe appended;
		tl = (tl_t*)(((uint8_t*)tl) + tlv_size);
	}
	
	packed_frame_destroy(to_unpack);
	
	if (rc < 0){
		frame_destroy(&to_return);
		return NULL;
	}
	return to_return;
}

bool packed_frame_verify_crc(packed_frame_t * frame){
	uint32_t crc = crc32(0, NULL, 0);
	crc = crc32(crc, frame->payload, sizeof(frame->payload));
	if (crc != frame->hdr.crc){
		printf("WARNING: crc mismatch 0x%08x expected, 0x%08x received\n", frame->hdr.crc, crc);
		return false;
	}
	return true;
}

void packed_frame_destroy(packed_frame_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying packed_frame_t at 0x%p\n", (void*)*to_destroy);
#endif
	free(*to_destroy);
	*to_destroy = NULL;
	return;		
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// radio_frame_t definition start////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

radio_frame_t * radio_frame(size_t len){
	radio_frame_t * to_return = (radio_frame_t*)malloc(sizeof(radio_frame_t));
	if (to_return == NULL) {return NULL;}
	
	to_return->frame_ptr = (uint8_t*)malloc(len);
	if (to_return->frame_ptr == NULL) {return NULL;}
	to_return->frame_len = len;
	to_return->frame_position = NULL;
	memset(to_return->frame_ptr, 0, to_return->frame_len);
	return to_return;	
}

int radio_frame_bytes_remaining(radio_frame_t * rf){
	if ((rf == NULL) || (rf->frame_position == NULL)){
		return -1;
	} else {
		return (int)(rf->frame_len - (rf->frame_position - rf->frame_ptr));
	}
}

bool radio_frame_transmit_start(radio_frame_t * frame, uint8_t channel){
#ifdef TX_START_DEBUG
	printf("INFO: radio_frame_transmit_start(%u)\n", frame->frame_len);
#endif
	
	if (frame->frame_ptr == NULL) {return false;} //nothing to transmit
	
	uint8_t new_state=0;
	
	//while (new_state != SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY){
	//	si446x_change_state(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY); //change to ready state
	//	zclock_sleep(1);
	//	new_state = radio_get_modem_state();
	//	//printf("radio->%s\n", radio_get_state_string(new_state));	
	//}
	
	frame->frame_position = frame->frame_ptr;
	
	si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT|SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT); // Reset TX FIFO
	si446x_get_int_status(0u, 0u, 0u); // Read ITs, clear pending ones

	// Fill the TX fifo with data
	if (frame->frame_len >= RADIO_MAX_PACKET_LENGTH){ // Data to be sent is more than the size of TX FIFO
		si446x_write_tx_fifo(RADIO_MAX_PACKET_LENGTH, frame->frame_position);
		frame->frame_position += RADIO_MAX_PACKET_LENGTH;
	}else{ // Data to be sent is less or equal than the size of TX FIFO
		si446x_write_tx_fifo(frame->frame_len, frame->frame_position);
		frame->frame_position += frame->frame_len;
	}
	
	//printf("start tx\n");
	
	// Start sending packet, channel 0, START immediately, Packet length of the frame
	si446x_start_tx(channel, 0x30,  frame->frame_len);

	//while (new_state != SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_TX){
	//	si446x_start_tx(channel, 0x00,  frame->frame_len);
	//	//si446x_start_tx(channel, 0x30,  frame->frame_len);
	//	new_state = radio_get_modem_state();
	//	printf("radio->%s\n", radio_get_state_string(new_state));
	//	zclock_sleep(1);
	//	printf("radio->%s\n", radio_get_state_string(radio_get_modem_state()));
	//}
	//printf("started tx\n");
	
	return true;
}

bool radio_frame_receive_start(radio_frame_t * frame){
	if (frame->frame_ptr == NULL) {return false;} //no memory allocated to receive
	memset(frame->frame_ptr, 0, frame->frame_len);
	frame->frame_position = frame->frame_ptr;
	return true;
}

void radio_frame_reset(radio_frame_t * frame){
	frame->frame_position = NULL;
	memset(frame->frame_ptr,0,frame->frame_len);
}

packed_frame_t * radio_frame_decode(radio_frame_t * to_decode){
	if (to_decode->frame_len != sizeof(coded_block_t)) {
		printf("ERROR: radio_frame_decode() length check failed!\n");
		return NULL;
	}
	
	packed_frame_t * to_return = (packed_frame_t*)malloc(sizeof(packed_frame_t));
	if (to_return == NULL) {return NULL;}
	
	uint8_t * decoded = turbo_encoder_decode(global_state->encoder, (uint8_t*)to_decode->frame_ptr, sizeof(coded_block_t));
	if (decoded == NULL){
	//if(!turbo_wrapper_decode((coded_block_t*)to_decode->frame_ptr, (uncoded_block_t*)to_return)){
		free(to_return);
		return NULL;
	}
	memcpy((uint8_t*)to_return, decoded, sizeof(packed_frame_t));
	//frame_header_print(&to_return->hdr);
	//printArrHex(to_return->payload, sizeof(to_return->payload));

	if (!packed_frame_verify_crc(to_return)){
		free(to_return);
		return NULL;
	}
	
	radio_frame_reset(to_decode);
	return to_return;
}

void radio_frame_destroy(radio_frame_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying radio_frame_t at 0x%p\n", (void*)*to_destroy);
#endif
	if ((*to_destroy)->frame_ptr != NULL){
		free((*to_destroy)->frame_ptr);
	}
	free(*to_destroy);
	*to_destroy = NULL;
	return;	
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// peer_t definition start//////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

peer_t * create_peer_from_beacon(beacon_t * beacon){
	peer_t * to_return = (peer_t*)malloc(sizeof(peer_t));
	if (to_return == NULL) {return NULL;}
	memset(to_return,0,sizeof(peer_t));
	peer_update_beacon(to_return, beacon);
	return to_return;
}

peer_t * create_peer_from_header(frame_header_t * hdr){
	peer_t * to_return = (peer_t*)malloc(sizeof(peer_t));
	if (to_return == NULL) {return NULL;}
	memset(to_return,0,sizeof(peer_t));
	peer_update_header(to_return, hdr);
	return to_return;
}

void peer_update_beacon(peer_t * p, beacon_t * b){
	memcpy(p->callsign, b->callsign, sizeof(p->callsign));
	memcpy(p->ip, b->ip, sizeof(p->ip));
	memcpy(p->mac, b->mac, sizeof(p->mac));
	p->last_update_ms = zclock_mono();
	p->ip_valid = true;
	
	if ((p->slip==false) && (p->link == true)){
		if (!peer_open_slip(p)){
			printf("ERROR: peer_open_slip() failed\n");
		}
	}
}

void peer_update_header(peer_t * p, frame_header_t * hdr){
	memcpy(p->callsign, hdr->callsign, sizeof(p->callsign));
	p->mac[0] = MAC_0;
	p->mac[1] = MAC_1;
	p->mac[2] = MAC_2;
	memcpy(&p->mac[3], hdr->mac_tail, sizeof(hdr->mac_tail));
	p->last_update_ms = zclock_mono();
}

bool peer_check_stale(peer_t * p){
	return ((zclock_mono()-p->last_update_ms) > global_state->link_peer_timeout_ms);
}

void peer_print(peer_t * p){
	printf("%.*s %u.%u.%u.%u %02x:%02x:%02x:%02x:%02x:%02x %lld\n", sizeof(p->callsign), p->callsign,
		p->ip[0], p->ip[1], p->ip[2], p->ip[3],
		p->mac[0],p->mac[1],p->mac[2],p->mac[3],p->mac[4],p->mac[5],
		p->last_update_ms
	);
}

char * peer_callsign(peer_t * p){
	static char callsign[10];
	snprintf(callsign, sizeof(callsign), "%.*s", sizeof(p->callsign), p->callsign);
	return callsign;
}

char * peer_ip_str(peer_t * p){
	static char ip_str[16];
	snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", p->ip[0],p->ip[1],p->ip[2],p->ip[3]);
	return ip_str;
}

char * peer_mac_str(peer_t * p){
	static char mac_str[20];
	snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
		p->mac[0],p->mac[1],p->mac[2],p->mac[3],p->mac[4],p->mac[5]
	);
	return mac_str;	
}

bool peer_ip_valid(peer_t * p){
	return p->ip_valid;
}

bool peer_open_slip(peer_t * p){
	if (!p->ip_valid){
		printf("WARNING: slip port for peer cannot be opened due to lack of valid IP\n");
		return false;
	}
	
	char str[128];
	snprintf(str, sizeof(str), "python3 /bridge/scripts/create_slip.py -d %s", peer_ip_str(p));
	system(str);
	p->slip = true;
	return true;
}

void peer_destroy(peer_t ** to_destroy){
#ifdef OBJECT_DESTROY_DEBUG
    printf("destroying peer_t at 0x%p\n", (void*)*to_destroy);
#endif
	free(*to_destroy);
	*to_destroy = NULL;
	return;		
}
