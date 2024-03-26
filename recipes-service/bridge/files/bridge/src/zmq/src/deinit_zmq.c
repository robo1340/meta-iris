/*
 * deinit_zmq.c
 *
 *  Created on: Apr 21, 2021
 *      Author: c53763
 */
#include <zmq.h>

#include "zmq_public_data.h"
#include "debug.h"

void deinit_zmq(void){
	zmq_close(zmq_sock_sub);
	zmq_close(zmq_sock_pub);
    zmq_close(zmq_sock_rep);
    zmq_ctx_destroy (zmq_ctx);
}
