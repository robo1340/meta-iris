/*!
 * File:
 *  radio_comm.h
 *
 * Description:
 *  This file contains the RADIO communication layer.
 *
 * Silicon Laboratories Confidential
 * Copyright 2012 Silicon Laboratories, Inc.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <czmq.h>

#include "print_util.h"
#include "radio.h"
#include "gpio.h"

//uint8_t Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY; //from radio_config.h
//radio_configuration_t RadioConfiguration = RADIO_CONFIGURATION_DATA; //from radio_config.h
//radio_configuration_t * radio_config_ptr = &RadioConfiguration;

static void radio_power_up(void){
	printf("INFO: radio_power_up()\n");
	si446x_reset(); // Hardware reset the chip 
	zclock_sleep(RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET/1000); // Wait until reset timeout
}

static bool radio_init_meta(zhashx_t * radio_config, zlistx_t * si4463_load_order){
	radio_power_up();
	
	radio_print_part_info();
	radio_print_func_info();
	
	if (SI446X_SUCCESS != si446x_apply_patch()){
		return false;
	}
	
	// Load radio configuration
	if (!si446x_configuration_init_enhanced(radio_config, si4463_load_order)){
		return false;
	}
	si446x_get_int_status(0u, 0u, 0u); // Read ITs, clear pending ones
	return true;	
}

bool radio_init(zhashx_t * radio_config, zlistx_t * si4463_load_order){
	printf("INFO radio_init()\n");
	if (!radio_hal_init("/dev/spidev0.0")){
		printf("ERROR: radio_hal_init() failed!\n");
	}
	
	bool rc = false;
	while (!rc){
		rc = radio_init_meta(radio_config, si4463_load_order);
	}
	return true;
}

bool radio_print_part_info(void){
	si446x_part_info();
	printf("si446x_part_info()->chiprev:0x%02x, PN:%x, PBUILD:%u, ID:%u, CUSTOMER:%u, ROMID:%u\n", 
		Si446xCmd.PART_INFO.CHIPREV, 
		Si446xCmd.PART_INFO.PART, 
		Si446xCmd.PART_INFO.PBUILD,
		Si446xCmd.PART_INFO.ID, 
		Si446xCmd.PART_INFO.CUSTOMER, 
		Si446xCmd.PART_INFO.ROMID
	);
	return true;
}

bool radio_print_func_info(void){
	si446x_func_info();
	uint8_t patch[2];
	memcpy(patch, &Pro2Cmd[3], sizeof(patch));
	printf("si446x_func_info()->revext:0x%02x, rev branch:0x%02x, rev int:0x%02x, func:0x%02x, patch:0x%02x%02x\n",
		Si446xCmd.FUNC_INFO.REVEXT,
		Si446xCmd.FUNC_INFO.REVBRANCH,
		Si446xCmd.FUNC_INFO.REVINT,
		Si446xCmd.FUNC_INFO.FUNC,
		patch[0], patch[1]
	);
	return true;
}

char * radio_get_state_string(uint8_t state){
	static char state_str[12];
	switch (state){
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_SLEEP:
			snprintf(state_str, sizeof(state_str), "SLEEP"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_SPI_ACTIVE:
			snprintf(state_str, sizeof(state_str), "SPI_ACTIVE"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY:
			snprintf(state_str, sizeof(state_str), "READY"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY2:
			snprintf(state_str, sizeof(state_str), "READY2"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_TX_TUNE:
			snprintf(state_str, sizeof(state_str), "TX_TUNE"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_RX_TUNE:
			snprintf(state_str, sizeof(state_str), "RX_TUNE"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_TX:
			snprintf(state_str, sizeof(state_str), "TX"); break;
		case SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_RX:
			snprintf(state_str, sizeof(state_str), "RX"); break;
		default:
			snprintf(state_str, sizeof(state_str), "unknown %u", Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE); break;
	}
	return state_str;
}

bool radio_print_state(bool print_if_no_change){
	static uint8_t prev_state;
	
	si446x_request_device_state();
	
	if ((Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE == prev_state) && (!print_if_no_change)){
		return true;
	} else if (Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE == 0){
		return true;
	}
	prev_state = Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE;
	
	printf("INFO: STATE=%s,CH=%u\n", 
		radio_get_state_string(Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE),
		Si446xCmd.REQUEST_DEVICE_STATE.CURRENT_CHANNEL
	);
	return true;
}

#define RSSI_CHANGE_TOLERANCE 20

bool radio_print_modem_status(bool print_if_no_change){
	static uint8_t prev_rssi = 0;
	
	si446x_get_modem_status(0xff);
	
	if ((Si446xCmd.GET_MODEM_STATUS.CURR_RSSI >= (prev_rssi-RSSI_CHANGE_TOLERANCE)) && 
	    (Si446xCmd.GET_MODEM_STATUS.CURR_RSSI <= (prev_rssi+RSSI_CHANGE_TOLERANCE))){
		return true;
	}
	prev_rssi = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;
	
	printf("INFO: CURR_RSSI:%u, LATCH_RSSI:%u\n", 
		Si446xCmd.GET_MODEM_STATUS.CURR_RSSI,
		Si446xCmd.GET_MODEM_STATUS.LATCH_RSSI
	);
		
    //Si446xCmd.GET_MODEM_STATUS.ANT1_RSSI    = Pro2Cmd[4];
    //Si446xCmd.GET_MODEM_STATUS.ANT2_RSSI    = Pro2Cmd[5];
    //Si446xCmd.GET_MODEM_STATUS.AFC_FREQ_OFFSET =  ((U16)Pro2Cmd[6] << 8) & 0xFF00;
    //Si446xCmd.GET_MODEM_STATUS.AFC_FREQ_OFFSET |= (U16)Pro2Cmd[7] & 0x00FF;
	return true;
}

uint8_t radio_get_modem_state(void){
	si446x_frr_a_read(1);
	return Si446xCmd.FRR_A_READ.FRR_A_VALUE;
}

uint8_t radio_get_rssi(void){
	si446x_get_modem_status(0xff);
	return Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;
}

bool radio_start_rx(uint8_t channel, size_t recv_len){
	//si446x_get_int_status(0u, 0u, 0u);
	si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT|SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT); //Reset FIFO
	si446x_get_int_status(0u, 0u, 0u); // Read ITs, clear pending ones

	// Start Receiving packet, channel 0, START immediately
	// this will change the radio state to rx but the program state will not go to rx until a sync word is detected
	si446x_start_rx(channel, 0u, recv_len,
		SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
		SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_READY,
		SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_READY
	);
	return true;		
	
}

static int hexstr_to_arr(char * hex_str, uint8_t * out_buf, size_t buf_len){
	uint16_t j;
	char * hex = hex_str;
	char * next;
	
	for (j=0; *hex!='\0';){
		
		if ((*hex==' ')||(*hex==',')){ 
			hex++;
			continue;
		}
		if ((*hex=='\0')||(*hex=='\r')||(*hex=='\n')){
			break;
		}
		
		//expect next characters in the string to be of the form "0x04," or "0x04"
		out_buf[j] = strtoul(hex, &next, 0); //let strtoul figure out that this is a hex string
		hex = next;
		j++;
		if (j>buf_len){
			printf("Error in convert_hex_string_to_array() buffer overflow\n");
			return -1;
		}
	}
	return (int)j;
}

zhashx_t * radio_load_configs(char * config_dir_path, zhashx_t * current_config, char * regex){
	zhashx_t * to_return = current_config;
	if (current_config == NULL){
		to_return = zhashx_new();
		if (to_return == NULL){return NULL;}
		zhashx_set_key_destructor(to_return, (zhashx_destructor_fn*) zstr_free);
		zhashx_set_key_duplicator(to_return, (zhashx_duplicator_fn*) strdup);
		zhashx_set_destructor(to_return, (zhashx_destructor_fn*)zchunk_destroy);
	}

	zdir_t * config_dir = zdir_new(config_dir_path, NULL);
	if (config_dir == NULL){return NULL;}
	
	printf("INFO radio_load_configs(\"%s\") %u children found\n", zdir_path(config_dir), zdir_count(config_dir));
	
	zlist_t * child_files = zdir_list(config_dir);
	zfile_t * child = zlist_first(child_files);
	while (child != NULL){
		if (zfile_is_directory(child)){
			to_return = radio_load_configs((char*)zfile_filename(child,NULL), to_return, regex);
		} else if (zfile_is_readable(child)){
			to_return = radio_parse_config((char*)zfile_filename(child,NULL), to_return, regex);
		}
		//if (to_return == NULL){return NULL;}
		child = zlist_next(child_files);
	}
	
	zdir_destroy(&config_dir);
	return to_return;
}

zhashx_t * radio_parse_config(char * path, zhashx_t * current_config, char * regex){
	printf("INFO radio_parse_config(\"%s\")\n", path);
	static char line_buf[512];
	char * line = line_buf;
	FILE * fp;
	size_t len = 0;
	int rc;
	char property[64];
	char value[256];
	uint8_t buf[32];
	zchunk_t * new_prop;
	
	zhashx_t * to_return = current_config;
	if (current_config == NULL){
		to_return = zhashx_new();
		if (to_return == NULL){return NULL;}
		zhashx_set_key_destructor(to_return, (zhashx_destructor_fn*) zstr_free);
		zhashx_set_key_duplicator(to_return, (zhashx_duplicator_fn*) strdup);
		zhashx_set_destructor(to_return, (zhashx_destructor_fn*)zchunk_destroy);
	}

	//example input for regex
	//#define RF_GLOBAL_XO_TUNE_2 0x11, 0x00, 0x02, 0x00, 0x52, 0x00
	zrex_t *rex = zrex_new (regex);
	if (!zrex_valid (rex)) {return NULL;}

	fp = fopen(path, "r");
	if (fp == NULL) {
		zrex_destroy (&rex);
		return NULL;
	}

	while (getline(&line, &len, fp) != -1){
		if (!zrex_matches (rex, line)) {continue;}
		strncpy(property, zrex_hit(rex,1), sizeof(property));
		strncpy(value, zrex_hit(rex,2), sizeof(value));
		
		//printf("\"%s\":\"%s\"\n", property, value);
		
		rc = hexstr_to_arr(value, buf, sizeof(buf));
		if (rc < 0){return NULL;}
		else if (rc > 0) {
			new_prop = zchunk_new(buf, rc);
			if (new_prop == NULL) {return NULL;}
			if (zhashx_lookup(to_return, property) != NULL){
				printf("WARNING: overwriting property \"%s\"\n",property);
			} else {
				zhashx_update(to_return, property, new_prop);
			}
		}
		
		zrex_destroy (&rex);
		rex = zrex_new (RADIO_CONFIG_REGEX);
		
	}
	fclose(fp);
	zrex_destroy (&rex);
	return to_return;
}


