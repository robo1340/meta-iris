/*
 * zmq_public_data.c
 *
 *  Created on: Jun 8, 2021
 *      Author: c53763
 */
#include "zmq_public_data.h"
#include "debug.h"

void *zmq_ctx;
void *zmq_sock_sub; //socket to set to ZMQ_SUB
void *zmq_sock_pub; //socket to set to ZMQ_PUB
void *zmq_sock_rep; //socket for exec responses

zmq_pollitem_t items[2];
