/*
 * zmq_public_data.h
 *
 *  Created on: Jun 8, 2021
 *      Author: c53763
 */

#ifndef SHARED_LIBS_INC_ZMQ_PUBLIC_DATA_H_
#define SHARED_LIBS_INC_ZMQ_PUBLIC_DATA_H_

#include <ucfg-util/ucfg-util.h>
#include "zmq.h"

/*** JSMN defines ***/
#define MAX_TOKENS	8	//Max number of key-value pairs

/*** Request/Response defines ***/
//#define REQREP_PROTOCOL_VER		"v1.0.0"
#define REQREP_CMD_MAX			32
#define REQREP_PAY_MAX			1024
#define MAX_COMMAND_LEN	32
#define MAX_PAYLOAD_LEN 1024

extern void *zmq_ctx;
extern void *zmq_sock_sub; //socket to set to ZMQ_SUB
extern void *zmq_sock_pub; //socket to set to ZMQ_PUB
extern void *zmq_sock_rep; //socket for exec responses

extern zmq_pollitem_t items[2];

/*** Envelope Indices ***/
/* Universal Envelope Indices */
#define ENV_BOX				0	/* aresbox number: 0 hardcoded until multi-ares is supported   */
#define ENV_APP     		1	/* applet node index */
/* Envelope indices for TYPE_LABEL_FLOW_ENV */
#define ENV_SPARE			2	/* spare node in case we need it */
#define ENV_POLEMOS			3	/* polemosboard number: from the config file polemos_equipment_table      */
#define ENV_TYPE			4	/* type: a429, a717, UART, CAN, DIO, etc.                                 */
#define ENV_CHAN			5	/* polemos channel of origin */
#define ENV_ID 				6	/* a429 label, a717 frame, CAN ID, etc */
/* Envelope indices: executive publishing tree */
#define ENV_PARAM_ID		2  /* Tree: Box = 0, App = 1, PARAM_ID = variable */
#define ENV_STATE_ID		3	/* If publishing a state change, PARAM_ID == 0 and next array index indicates the change */

/*** Envelope Values ***/
/* Envelope values: box node. */
#define ENV_BOX_0	 		0	/* Index of the ares box: AReS 3 V1. Assigned to the "ENV_BOX" array element. */
/* Envelope values: applet node */
#define ENV_APP_PUB			0	/* data ingester/publisher applet */
#define ENV_APP_EXE			1	/* executive applet */
#define ENV_APP_REC			2	/* data media recorder applet*/
#define ENV_APP_EVENT		3	/* event detector applet */
#define ENV_APP_NETW		4	/* network manager applet */
/* Envelope values: spare "channel tree" node (Applies to Polemos data tree only) */
#define ENV_SPARE_1			1	/* spare is always == 1 */
/* Envelope values: Executive Pub */
#define ENV_STATE_CHANGE	0   /* State changes are always published as param 0 */
#define ENV_SET_ID		0	/* id.cfg updated */
#define ENV_SET_CFG		1	/* Configuration file updated */
#define ENV_SET_SRL		2	/* serial.cfg updated */

#endif
