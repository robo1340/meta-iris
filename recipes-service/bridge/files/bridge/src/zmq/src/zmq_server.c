/*
 * zmq_req_server.c
 *
 *  Created on: Aug 1, 2022
 *      Author: c53763
 */
#include "zmq_server.h"
#include "system_error_utils.h"
#include "debug.h"

#include <zmq.h>
#include <czmq.h>
#include <string.h>
#include "jsmn.h"
#include "ipc_proxy.h"
#include "zmq_public_data.h"
#include "polemos.h"
#include "envelope_util.h"
#include "polemos_ft_tests_public.h"

#include "ingest_eventdetector_param.h"
#include "ingest_recorder_param.h"

#include <stdio.h>
#include <ucfg-util/ucfg-util.h>
#include "system_config.h"
#include "executive_global_state.h"

#define IPC_ARES_EXEC_VERSION "v1.0.0"

/** @brief receive a 2-part zmq message from a socket, if a message with greater than two frames is received, all other frames will be dropped
 *  @param sock, a zmq socket that is open and has a message ready to receive
 *  @param env, a pointer to an envelope buffer
 *  @param env_len, a pointer to the size of the envelope buffer, on return env_len will be set to the size of envelope received
 *  @param pay, a pointer to a payload buffer
 *  @param pay_len, a pointer to the size of the payload buffer, on return pay_len will be set to the size of payload received
 *  @return returns true on success, returns false on failure
 */
bool receive_zmq_msg(void * sock, uint8_t * env, size_t * env_len, uint8_t * pay, size_t * pay_len);

/* Globals */
static uint8_t env_buf[ENV_BUF_SZ_MAX]; //a buffer for the serialized envelope
static size_t env_len;
static uint8_t pay_buf[PAY_BUF_SZ_MAX]; //a buffer for the message payload
static size_t pay_len;

bool handle_get_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);
bool handle_set_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);
bool handle_clear_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);
bool handle_set_configuration(char *rep_buf, size_t rep_buf_size, char *buf, size_t buf_size);
bool handle_clear_configuration(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);
bool handle_get_config(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);
bool handle_get_gpio(char * rep_buf, size_t rep_buf_size, char * buf, size_t buf_size);
bool handle_set_gpio(char * rep_buf, size_t rep_buf_size, char *buf, size_t buf_size);
bool handle_run_polemos_ft_test(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size);

void zmq_server(void) {
	static char rep_buf[MAX_PAYLOAD_LEN];
	
	if(zmq_poll(items, 2, 10) < 0){
		DBUG;
	}
	bool status = true;
	rep_buf[0] = '\0'; // null-terminate the response buffer for string handling safety

	//static void 
	//static void executive_publish_config_change(executive * exec){

	//Read messages
	if (items[0].revents & ZMQ_POLLIN){
		char * client_id = zstr_recv(zmq_sock_rep);
		char * empty = zstr_recv(zmq_sock_rep);
		char * version = zstr_recv(zmq_sock_rep);
		char * command = zstr_recv(zmq_sock_rep);
		char * payload  = zstr_recv(zmq_sock_rep);

		if(client_id && empty && version && command && payload){
			if ((strlen(version) >= MAX_VERSION_STRLEN) || (strlen(command) >= MAX_COMMAND_STRLEN) || (strlen(payload) >= MAX_PAYLOAD_STRLEN)){
				DBUG;
				status = false;
			}
			printf("request: \"%s\"\n", command);
			/* handle request
			 * Note: The if-else logic could be optimized
			 * using strcmp() greater than or less than, by ordering
			 * the text command strings in order greatest to smallest.
			 */
			if (strcmp(command, "GET_AIRCRAFT_ID") == 0){
				if (!handle_get_aircraft_id(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_get_aircraft_id() failed!\n");
					status = false;
				}
			}
			else if (strcmp(command, "CLEAR_AIRCRAFT_ID") == 0){
				if (!handle_clear_aircraft_id(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_clear_aircraft_id() failed!\n");
					status = false;
				}
			}
			else if (strcmp(command, "UPDATE_AIRCRAFT_ID") == 0){
				if (!handle_set_aircraft_id(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_set_aircraft_id() failed!\n");
					status = false;
				} else {
					executive_publish_aircraft_id_change(global_state_ptr);
				}
			}
			else if (strcmp(command, "CLEAR_CONFIGURATION") == 0){
				if (!handle_clear_configuration(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_clear_configuration() failed!\n");
					status = false;
				} else {
					executive_publish_config_change(global_state_ptr);
				}
			}
			else if (strcmp(command, "SET_CONFIGURATION") == 0){
				if (!handle_set_configuration(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_set_configuration() failed!\n");
					status = false;
				} else {
					executive_publish_config_change(global_state_ptr);
				}
			}
			else if (strcmp(command, "GET_CONFIG") == 0){
				if (!handle_get_config(rep_buf, sizeof(rep_buf), payload, strlen(payload))){
					printf("WARNING handle_get_config() failed!\n");
					status = false;
				}
			}
			else if (strcmp(command, "GPIO_GET") == 0){
				if(!handle_get_gpio(rep_buf,sizeof(rep_buf),payload,strlen(payload))){
					status = false;
					printf("handle_set_gpio() failed!\n");
				}
			}
			else if (strcmp(command, "GPIO_SET") == 0){
				if(!handle_set_gpio(rep_buf,sizeof(rep_buf),payload,strlen(payload))){
					status = false;
					printf("handle_set_gpio() failed!\n");
				}
			}
			else if (strcmp(command, "RUN_POLEMOS_FT_TEST") == 0){
				DBUG_V(payload,"%s");
				if(!handle_run_polemos_ft_test(rep_buf,sizeof(rep_buf),payload,strlen(payload))){
					status = false;
					printf("handle_run_polemos_ft_test() failed!\n");
				}
			}else {
				snprintf(rep_buf, sizeof(rep_buf), "unkown command %s", command);
				printf("WARNING: unknown command %s\n", command);
				status = false;
			}
		}
		else {
			snprintf(rep_buf, sizeof(rep_buf), "failure to receive message");
			printf("WARNING: failure to receive message\n");
			status = false;
		}

		//printf("%s\n", client_id);
		//printf("%s\n", IPC_ARES_EXEC_VERSION);
		//printf((status==true) ? "ACK\n" : "NACK\n");
		//printf("%s\n", rep_buf);

		zstr_sendm(zmq_sock_rep, client_id); //destination client
		zstr_sendm(zmq_sock_rep, ""); //
		zstr_sendm(zmq_sock_rep, IPC_ARES_EXEC_VERSION); //
		zstr_sendm(zmq_sock_rep, ((status==true) ? "ACK" : "NACK")); //
		zstr_send(zmq_sock_rep, rep_buf);

		//Cleanup zmq structures
		zstr_free(&client_id);
		zstr_free(&empty);
		zstr_free(&version);
		zstr_free(&command);
		zstr_free(&payload);
	}
	else if (items[1].revents & ZMQ_POLLIN) { // Receive messages from subscriber socket
		env_len = sizeof(env_buf);
		pay_len = sizeof(pay_buf);
		if (receive_zmq_msg(zmq_sock_sub, env_buf, &env_len, pay_buf, &pay_len)){
			if (env_buf[ENV_APP] == ENV_APP_REC){
				ingest_recorder_param(env_buf, env_len, pay_buf, pay_len);
			}
			else if (env_buf[ENV_APP] == ENV_APP_EVENT){
				if (!ingest_eventdetector_param(env_buf, env_len, pay_buf, pay_len)){
					printf("ingest_eventdetector_param() failed!\n");
				}
			}
		}
	}
}

bool receive_zmq_msg(void * sock, uint8_t * env, size_t * env_len, uint8_t * pay, size_t * pay_len){
	int rc;
	int more_frames;
	size_t more_len = sizeof(more_frames);
	
	rc = zmq_recv(sock, env, *env_len, 0);
	if (rc < 0){
		perror("recv envelope");
		return false;
	} else {
		*env_len = (size_t)rc;
	}
	
	zmq_getsockopt(sock,ZMQ_RCVMORE,&more_frames,&more_len);
	if (more_frames == ZMQ_MORE){
		rc = zmq_recv(sock, pay, *pay_len, 0);
		if (rc < 0) {
			perror("recv payload");
			return false;	
		} else {
			*pay_len = (size_t)rc;
		}
	}
	
	//receive any other frames after the second and throw them away, comment this part out if it isn't desired
	zmq_getsockopt(sock,ZMQ_RCVMORE,&more_frames,&more_len);
	while (more_frames == ZMQ_MORE){
		rc = zmq_recv(sock, NULL, 0, 0);
		if (rc < 0) {break;}
		zmq_getsockopt(sock,ZMQ_RCVMORE,&more_frames,&more_len);
	}
	
	return true;
}

bool handle_get_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size) {
	printf("handle_get_aircraft_id()\n");

	if (ucfg_global_ptr == NULL) {
		snprintf(rep_buf,rep_buf_size, "No Aircraft ID or Config Loaded");
		return false;
	}
	
	snprintf(rep_buf, rep_buf_size, "{\n\t\"company\":\"%s\",\n\t\"model\":\"%s\",\n\t\"serial\":\"%s\",\n\t\"field1\":\"%s\",\n\t\"field2\":\"%s\",\n\t\"era\":\"%s\"\n}", 
		ucfg_global_ptr->id.company, 
		ucfg_global_ptr->id.model, 
		ucfg_global_ptr->id.serial, 
		ucfg_global_ptr->id.field1, 
		ucfg_global_ptr->id.field2, 
		ucfg_global_ptr->id.era_uuid);
	return true;
}

bool handle_set_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size) {
	printf("handle_set_aircraft_id()\n");
	printf("%s\n", payload);
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[16]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[80];
	
	ucfg_aircraft_identity_t new_id;
	memset(&new_id, 0, sizeof(new_id));
	
	bool model_found = false; //required field
	bool serial_found = false;//required field
	bool company_found = false;//required field
	bool force = false; //optional field
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, payload, payload_size, tokens, sizeof(tokens)/sizeof(jsmntok_t));
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){snprintf(rep_buf, rep_buf_size, "%s", "bad token, JSON string is corrupted"); return false;}
		else if (num_toks == JSMN_ERROR_NOMEM){snprintf(rep_buf, rep_buf_size, "%s", "not enough tokens, JSON string is too large"); return false;}
		else if (num_toks == JSMN_ERROR_PART){snprintf(rep_buf, rep_buf_size, "%s", "JSON string is too short, expecting more JSON data"); return false;}
	    return false;
	}
	else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {snprintf(rep_buf, rep_buf_size, "%s", "payload received is not a json object!"); return false;}
		if (num_toks < 2) {snprintf(rep_buf, rep_buf_size, "%s", "not enough tokens found in json object!"); return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, payload+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, payload+tokens[j].start);
			//printf("%s %s\n", key, value);
			if (strcmp(key, "model") == 0){
				strncpy(new_id.model, value, sizeof(new_id.model));
				model_found = true;
			}
			else if (strcmp(key, "serial") == 0){
				strncpy(new_id.serial, value, sizeof(new_id.serial));
				serial_found = true;
			}
			else if (strcmp(key, "company") == 0){
				strncpy(new_id.company, value, sizeof(new_id.company));
				company_found = true;
			}
			else if (strcmp(key, "field1") == 0){
				strncpy(new_id.field1, value, sizeof(new_id.field1));
			}
			else if (strcmp(key, "field2") == 0){
				strncpy(new_id.field2, value, sizeof(new_id.field2));
			}
			else if (strcmp(key, "force") == 0){ //if true force the id change even if an id is set
				if (strcmp(value, "true") == 0){force = true;}
			}
		}
	}
	
	if (!model_found)  {snprintf(rep_buf, rep_buf_size, "%s", "required field \"model\" not included!"); return false;}
	if (!serial_found) {snprintf(rep_buf, rep_buf_size, "%s", "required field \"serial\" not included!"); return false;}
	if (!company_found) {snprintf(rep_buf, rep_buf_size, "%s", "required field \"company\" not included!"); return false;}
	
	if (force == true) {printf("WARNING: \"force\" option specified, current aircraft id will be cleared\n");}
	else if (ucfg_aircraft_id_loaded(ucfg_global_ptr) && (!force)){
		snprintf(rep_buf, rep_buf_size, "%s", "aircraft id is already set run command CLEAR_AIRCRAFT_ID first or specify \"force:true\" in payload");
		return false;
	}
	
	if (!executive_set_aircraft_id(global_state_ptr, &new_id)){
		snprintf(rep_buf, rep_buf_size, "%s", "failed to set aircraft ID!, payload is syntactically correct though"); 
		return false;
	}
	return true;
}

bool handle_clear_aircraft_id(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size) {
	printf("handle_clear_aircraft_id()\n");
	
	ucfg_aircraft_identity_t new_id; //create a clear aircraft id
	memset(&new_id, 0, sizeof(new_id));
	
	if (!executive_set_aircraft_id(global_state_ptr, &new_id)){
		snprintf(rep_buf, rep_buf_size, "%s", "failed to clear aircraft ID"); 
		return false;
	}
	return true;
}

bool handle_set_configuration(char *rep_buf, size_t rep_buf_size, char *buf, size_t buf_size){
	printf("handle_set_configuration(\"%s\")\n", buf);
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[MAX_TOKENS]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[CFG_FILE_PATH_LEN];
	char * new_cfg_path = NULL;
	ucfg_t * current_config = ucfg_global_ptr; //save a pointer to our current config
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, buf, buf_size, tokens, MAX_TOKENS);
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){snprintf(rep_buf, rep_buf_size, "bad token, JSON string is corrupted");}
		else if (num_toks == JSMN_ERROR_NOMEM){snprintf(rep_buf, rep_buf_size, "not enough tokens, JSON string is too large");}
		else if (num_toks == JSMN_ERROR_PART){snprintf(rep_buf, rep_buf_size, "JSON string is too short, expecting more JSON data");}
	    return false;
	}
	else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {return false;}
		if (num_toks < 2) {return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, buf+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, buf+tokens[j].start);
			//printf("%s %s\n", key, value);
			if (strncmp(key, "version", 8) == 0){
				new_cfg_path = value;
			}
		}	
	}
	if (new_cfg_path == NULL){
		snprintf(rep_buf, rep_buf_size, "required key value pair \"version\" not present");
		return false;
	}
	
	//verify config loaded
	ucfg_global_ptr = ucfg_create(new_cfg_path); // attempt to load the config file passed in
	if (ucfg_global_ptr == NULL){
		snprintf(rep_buf, rep_buf_size, "failed to load new config at \"%s\"", new_cfg_path);
		return false;
	}
	
	//verify config model and aircraft id model match
	if (strncmp(ucfg_global_ptr->ac_id.model, ucfg_global_ptr->id.model, sizeof(ucfg_global_ptr->id.model)) != 0) {
		snprintf(rep_buf, rep_buf_size, "config loaded is for model \"%s\", aircraft id id current configured to be model \"%s\"", ucfg_global_ptr->ac_id.model, ucfg_global_ptr->id.model);
		return false;
	}
	
	//move forward with accepting the new config
	char path[CFG_FILE_PATH_LEN];
	snprintf(path, sizeof(path), "%s/%08X.cfg", UCFG_CONFIG_DIR_PATH, ucfg_data_version_int(ucfg_global_ptr));
	rename(new_cfg_path, path);
	symlink(path, UCFG_CONFIG_PATH);
	
	//destroy the old config object
	if (current_config != NULL){
		ucfg_destroy(&current_config);
	}
	
	system("sync"); //sync so the new config will get flushed to disk
	
	deinit_polemos(); //destroy and recreate all polemos connections so settins are updated
	init_polemos();
	
	//system("systemctl restart publisher");
	//system("systemctl restart recorder");
	executive_publish_config_change(global_state_ptr);
	executive_flutter_lamps(global_state_ptr);
	return true;
}

bool handle_clear_configuration(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size) {
	printf("handle_clear_configuration(\"%s\")\n", payload);
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[MAX_TOKENS]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[32];
	char * version = NULL;
	bool clearall = false;
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, payload, payload_size, tokens, MAX_TOKENS);
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){DBUG_T("bad token, JSON string is corrupted\n");}
		else if (num_toks == JSMN_ERROR_NOMEM){DBUG_T("not enough tokens, JSON string is too large\n");}
		else if (num_toks == JSMN_ERROR_PART){DBUG_T("JSON string is too short, expecting more JSON data\n");	}
	    return false;
	}
	else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {return false;}
		if (num_toks < 2) {return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, payload+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, payload+tokens[j].start);
			
			//printf("%s %s\n", key, value);
			if (strcmp(key, "version") == 0){
				version = value;
			}
			if (strcmp(key, "clearall") == 0){
				if (strcmp(value, "true") == 0){
					clearall = true;
				}
			}
		}	
	}
	
	if ((version != NULL) && (strcmp(version,"0xFFFFFFFF")==0)){
		clearall = true;
	}
	
	printf("clearing current config\n");
	unlink(UCFG_CONFIG_PATH);      // remove the current cfg file
	if (clearall == true) {
		printf("clearing all configs\n");
		char cmd[CFG_FILE_PATH_LEN];
		snprintf(cmd, sizeof(cmd), "rm -rf %s/*", UCFG_CONFIG_DIR_PATH);
		printf("%s", cmd);
		system(cmd);
	}

	if (ucfg_global_ptr != NULL){
		ucfg_destroy(&ucfg_global_ptr);
	}
	ucfg_global_ptr = ucfg_create(UCFG_DEFAULT_CONFIG_PATH);
	if (ucfg_global_ptr == NULL){
		printf("CRITICAL ERROR: could not read default config \"%s\"\n", global_state_ptr->config_file_path);
	}
	return true;
}

bool handle_get_config(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size){
	char   path[CFG_FILE_PATH_LEN];

	// Check if box is configured
	if ((ucfg_global_ptr == NULL) || (global_state_ptr->default_config_loaded)){ //no config or default config loaded
		snprintf(path, sizeof(path), "unconfigured");
	}
	else if (global_state_ptr->ft_mode == true) { //executive is in FT mode
		strncpy(path, UCFG_CONFIG_PATH, sizeof(path));	
	}
	else { //box is configured
		snprintf(path, sizeof(path), "%s/%08X.cfg", UCFG_CONFIG_DIR_PATH, ucfg_data_version_int(ucfg_global_ptr));
	}
	snprintf(rep_buf, rep_buf_size, "{\n\t\"path\" : \"%s\"\n}", path); //JSON format key-value pairs
	return true;
}

bool handle_get_gpio(char * rep_buf, size_t rep_buf_size, char * buf, size_t buf_size){
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[MAX_TOKENS]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[32];
	gpio_t * pin = NULL;
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, buf, buf_size, tokens, MAX_TOKENS);
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){snprintf(rep_buf, rep_buf_size, "bad token, JSON string is corrupted");}
		else if (num_toks == JSMN_ERROR_NOMEM){snprintf(rep_buf, rep_buf_size, "not enough tokens, JSON string is too large");}
		else if (num_toks == JSMN_ERROR_PART){snprintf(rep_buf, rep_buf_size, "JSON string is too short, expecting more JSON data");}
	    return false;
	} else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {return false;}
		if (num_toks < 2) {return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, buf+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, buf+tokens[j].start);
			
			//printf("%s %s\n", key, value);
			if (strcmp(key, "line") == 0){
				pin = (gpio_t*)zhashx_lookup(gpio_bank_global->bank, value);
				if (pin == NULL){
					snprintf(rep_buf, rep_buf_size, "invalid line selected \'%s\'\n", value);
					return false;
				}
			}
		}	
	}
	if (pin != NULL){
		bool state;
		if (gpio_get_value(pin, &state) == false){
			snprintf(rep_buf, rep_buf_size, "gpio get failed\n");
			return false;
		} else {
			snprintf(rep_buf, rep_buf_size, "{\"state\":\"%d\"}", (state==true)?1:0 ); //JSON format key-value pairs
			return true;
		}
	} else {return false;}
}

bool handle_set_gpio(char * rep_buf, size_t rep_buf_size, char *buf, size_t buf_size) {
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[MAX_TOKENS]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[32];
	
	gpio_t * pin = NULL;
	bool state;
	bool state_found = false;
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, buf, buf_size, tokens, MAX_TOKENS);
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){snprintf(rep_buf, rep_buf_size, "bad token, JSON string is corrupted");}
		else if (num_toks == JSMN_ERROR_NOMEM){snprintf(rep_buf, rep_buf_size, "not enough tokens, JSON string is too large");}
		else if (num_toks == JSMN_ERROR_PART){snprintf(rep_buf, rep_buf_size, "JSON string is too short, expecting more JSON data");}
	    return false;
	} else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {return false;}
		if (num_toks < 2) {return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, buf+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, buf+tokens[j].start);
			
			//printf("%s %s\n", key, value);
			if (strcmp(key, "line") == 0){
				pin = (gpio_t*)zhashx_lookup(gpio_bank_global->bank, value);
				if (pin == NULL){
					snprintf(rep_buf, rep_buf_size,"invalid line selected \'%s\'\n", value);
					return false;
				}
			}
			else if (strcmp(key, "state") == 0){
				if (value[0] == '0') {state = false;}
				else if (value[0] == '1') {state = true;}
				else {
					snprintf(rep_buf, rep_buf_size,"state value isnt appropriate\n");
					return false;
				}
				state_found = true;
			}
		}
	}
	
	if ((pin != NULL) && (state_found == true)){
		if (gpio_set_value(pin, state) == false){
			printf("gpio set failed\n");
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

#define DEFAULT_POLEMOS_IP "172.17.37.0"
bool handle_run_polemos_ft_test(char *rep_buf, size_t rep_buf_size, char * payload, size_t payload_size) {
	int i, j;
	jsmn_parser p;
	jsmntok_t tokens[MAX_TOKENS]; //Needs to be larger than two times max key value pairs plus 1
	char key[32];
	char value[32];
	
	char polemos_ip[20] = DEFAULT_POLEMOS_IP;	//optional field
	char test[16] = "";			//required field
	char arg1[16] = "";			//usually optional field
	char arg2[16] = "";			//usually optional field	
	
	//parse the json tokens
	jsmn_init(&p); //reset the json parser
	int num_toks = jsmn_parse(&p, payload, payload_size, tokens, MAX_TOKENS);
	
	if (num_toks < 0){
		if (num_toks == JSMN_ERROR_INVAL){snprintf(rep_buf, rep_buf_size, "%s", "bad token, JSON string is corrupted"); return false;}
		else if (num_toks == JSMN_ERROR_NOMEM){snprintf(rep_buf, rep_buf_size, "%s", "not enough tokens, JSON string is too large"); return false;}
		else if (num_toks == JSMN_ERROR_PART){snprintf(rep_buf, rep_buf_size, "%s", "JSON string is too short, expecting more JSON data"); return false;}
	    return false;
	}
	else { //start parsing the tokens
		//the first token needs to be a JSMN_OBJECT
		if (tokens[0].type != JSMN_OBJECT) {snprintf(rep_buf, rep_buf_size, "%s", "payload received is not a json object!"); return false;}
		if (num_toks < 2) {snprintf(rep_buf, rep_buf_size, "%s", "not enough tokens found in json object!"); return false;}
		
		//iterate over the key-value pairs in the payload
		for(i=1,j=2; j<num_toks; i+=2, j+=2){
			snprintf(key, sizeof(key),  "%.*s",tokens[i].end-tokens[i].start, payload+tokens[i].start);
			snprintf(value, sizeof(value),"%.*s",tokens[j].end-tokens[j].start, payload+tokens[j].start);
			
			//printf("%s %s\n", key, value);
			if (strcmp(key, "polemos_ip") == 0){
				strncpy(polemos_ip, value, sizeof(polemos_ip));
			}
			else if (strcmp(key, "test") == 0){
				strncpy(test, value, sizeof(test));
			}
			else if (strcmp(key, "arg1") == 0){
				strncpy(arg1, value, sizeof(arg1));
			}
			else if (strcmp(key, "arg2") == 0){
				strncpy(arg2, value, sizeof(arg2));
			}
		}
	}
	
	if (strlen(test) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "required field \"test\" not included!"); return false;}
	//handle test key-value pair now now
	bool to_return;
	system("systemctl stop publisher");
	zclock_sleep(250);
	if (strcmp(test, "version") == 0){
		if (strlen(arg1) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg1\" required for this test"); to_return = false;}
		if (strlen(arg2) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg2\" required for this test"); to_return = false;}
		to_return = run_polemos_version_test(polemos_ip, rep_buf, rep_buf_size, arg1, arg2);
	}
	else if (strcmp(test, "status") == 0){
		to_return = run_polemos_ft_status_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "blink") == 0){
		to_return = run_polemos_ft_blink_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "a429") == 0){
		to_return = run_polemos_ft_a429_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "sdram") == 0){
		to_return = run_polemos_ft_sdram_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "a717") == 0){
		to_return = run_polemos_ft_a717_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "discretes") == 0){
		to_return = run_polemos_ft_discrete_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "can") == 0){
		to_return = run_polemos_ft_can_test(polemos_ip, rep_buf, rep_buf_size);
	}
	else if (strcmp(test, "cdsb") == 0){
		if (strlen(arg1) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg1\" required for this test"); to_return = false;}
		to_return = run_polemos_ft_cdsb_test(polemos_ip, rep_buf, rep_buf_size, arg1);
	}
	else if (strcmp(test, "uart") == 0){
		if (strlen(arg1) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg1\" required for this test"); to_return = false;}
		if (strlen(arg2) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg2\" required for this test"); to_return = false;}
		to_return = run_polemos_ft_uart_test(polemos_ip, rep_buf, rep_buf_size, arg1, arg2);
	}
	else if (strcmp(test, "straps") == 0){
		if (strlen(arg1) == 0) {snprintf(rep_buf, rep_buf_size, "%s", "field \"arg1\" required for this test"); to_return = false;} 
		to_return = run_polemos_ft_strap_test(polemos_ip, rep_buf, rep_buf_size, arg1);
	}
	else {
		snprintf(rep_buf, rep_buf_size,"test \"%s\" is not supported", test); 
		to_return = false;
	}
	system("systemctl start publisher");
	zclock_sleep(100);
	return to_return;
}

