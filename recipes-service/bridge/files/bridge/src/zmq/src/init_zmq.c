/*
 * init_zmq_exec.c
 *
 *  Created on: 9/15/2023
 *      Author: c19358
 */
#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ipc_proxy.h>
#include "zmq_public_data.h"
#include "system_error_utils.h"
#include "init_zmq.h"
#include "executive_global_state.h"
#include "debug.h"

static bool init_rep(void);
static bool init_pub(void);
static bool init_sub(void);

void init_zmq(void) {
	printf("init_zmq()\n");

	if((zmq_ctx = zmq_ctx_new()) == NULL) {
		system_error_set(ZMQ_FATAL_FAULT);
		return;
	}

	if(!init_rep()) {
		printf("ERROR init_rep failed!");
		system_error_set(ZMQ_FATAL_FAULT);
	}
	if(!init_pub()) {
		printf("ERROR init_pub failed!");
		system_error_set(ZMQ_FATAL_FAULT);
	}
	if(!init_sub()) {
		printf("ERROR init_sub failed!");
		system_error_set(ZMQ_FATAL_FAULT);
	}
	else{
		// subscribe to recorder applet param tree
		uint8_t env[2] = {ENV_BOX_0, ENV_APP_REC};
		if(zmq_setsockopt (zmq_sock_sub, ZMQ_SUBSCRIBE, env, sizeof(env)) < 0) {system_error_set(ZMQ_FATAL_FAULT);}
		uint8_t env_evt[3] = {ENV_BOX_0, ENV_APP_EVENT, 0}; //subscribe to event detector parameters
		if(zmq_setsockopt (zmq_sock_sub, ZMQ_SUBSCRIBE, env_evt, sizeof(env_evt)) < 0) {system_error_set(ZMQ_FATAL_FAULT);}
	}
	
	//give the global state pointer references to the newly created zmq sockets
	global_state_ptr->zmq_sock_sub = zmq_sock_sub;
	global_state_ptr->zmq_sock_pub = zmq_sock_pub;
	global_state_ptr->zmq_sock_rep = zmq_sock_rep;
	
}

static bool init_rep(void) {
	printf("init_rep\n");
	int zmq_sock_timeout = 30000;

	if((zmq_sock_rep = zmq_socket(zmq_ctx, ZMQ_DEALER)) == NULL) {return false;} //Create a dealer socket
	if(zmq_setsockopt(zmq_sock_rep, ZMQ_RCVTIMEO, &zmq_sock_timeout, sizeof(zmq_sock_timeout)) < 0) {return false;}
	if(zmq_setsockopt(zmq_sock_rep, ZMQ_SNDTIMEO, &zmq_sock_timeout, sizeof(zmq_sock_timeout)) < 0) {return false;}

	if(zmq_bind(zmq_sock_rep, "ipc:///tmp/ares_exec_rep.ipc") < 0) {return false;}

	items[0].socket = zmq_sock_rep;
	items[0].fd = 0;
	items[0].events = ZMQ_POLLIN;
	items[0].revents = 0;

	return true;
}

static bool init_pub(void) {
	printf("init_pub\n");
	if((zmq_sock_pub = zmq_socket (zmq_ctx, ZMQ_PUB)) == NULL) {return false;} //Create a PUB socket
	if(zmq_connect (zmq_sock_pub, IPC_PROXY_SUB_ADDRESS) < 0) {return false;}
	return true;
}

static bool init_sub(void) {
	printf("init_sub\n");

	if((zmq_sock_sub = zmq_socket (zmq_ctx, ZMQ_SUB)) == NULL){return false; } // create a SUB socket
	if(zmq_connect(zmq_sock_sub, IPC_PROXY_PUB_ADDRESS) < 0){return false; } // ipc_proxy is the PUB to our SUB

	items[1].socket = zmq_sock_sub;
	items[1].fd = 0;
	items[1].events = ZMQ_POLLIN;
	items[1].revents = 0;

	return true;
}

