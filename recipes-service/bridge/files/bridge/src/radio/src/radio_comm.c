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
#include "radio_hal.h"
#include "radio_comm.h"
#include "si446x_cmd.h"

//#define RADIO_COMM_DEBUG
//#define LL_RADIO_COMM_DEBUG

uint8_t tx[64] = {0};
uint8_t rx[64] = {0};


uint8_t radio_comm_GetResp(uint8_t byteCount, uint8_t * pData){
#ifdef RADIO_COMM_DEBUG
	printf("radio_comm_GetResp()\n");
#endif
	if(!radio_comm_PollCTS()){return 0;}
	
	memset(tx, 0, 2+byteCount);
	memset(rx, 0, 2+byteCount);
	tx[0] = SI446X_CMD_ID_READ_CMD_BUFF;
	tx[1] = 0x00;
	radio_hal_transfer(tx, rx, 2+byteCount);
	memcpy(pData, &rx[2], byteCount);
	return 0xff;
}

/*!
 * Sends a command to the radio chip
 *
 * @param byteCount     Number of bytes in the command to send to the radio device
 * @param pData         Pointer to the command to send.
 */
void radio_comm_SendCmd(uint8_t byteCount, uint8_t * pData){
	if(!radio_comm_PollCTS()) {return;}
	radio_hal_SpiWriteData(byteCount, pData);
}

/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip.
 * @param pData         Pointer to where to put the data.
 */
void radio_comm_ReadData(uint8_t cmd, BIT pollCts, uint8_t byteCount, uint8_t * pData){
#ifdef RADIO_COMM_DEBUG
	printf("radio_comm_ReadData()\n");
#endif
	if (pollCts){
		if(!radio_comm_PollCTS()) {
			printf("ERROR: radio_comm_PollCTS() failed!\n");
			return;
		}
	}
	memset(tx, 0, sizeof(cmd)+byteCount);
	memset(rx, 0, sizeof(cmd)+byteCount);
	tx[0] = cmd;
	radio_hal_transfer(tx, rx, sizeof(cmd)+byteCount);
	memcpy(pData, &rx[1], byteCount);
	//printf("radio_comm_ReadData()->");
	//printArrHex(pData, byteCount);
}


/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip
 * @param pData         Pointer to where to put the data
 */
void radio_comm_WriteData(uint8_t cmd, BIT pollCts, uint8_t byteCount, uint8_t * pData){
#ifdef RADIO_COMM_DEBUG
	printf("radio_comm_WriteData()\n");
#endif
    if(pollCts){
		if(!radio_comm_PollCTS()) {return;}
    }
	tx[0] = cmd;
	memcpy(&tx[1], pData, byteCount);
	//printf("radio_comm_WriteData()->");
	//printArrHex(tx, sizeof(cmd)+byteCount);
	radio_hal_transfer(tx, rx, sizeof(cmd)+byteCount);
}

/*!
 * Waits for CTS to be high
 *
 * @return CTS value
 */
bool radio_comm_PollCTS(void){
	//uint16_t errCnt = RADIO_CTS_TIMEOUT;
	tx[0] = SI446X_CMD_ID_READ_CMD_BUFF;
	tx[1] = 0x00;
	memset(rx, 0, 2);

	while (rx[1] != 0xFF){
		if (radio_hal_transfer(tx, rx, 2) < 0){
			printf("ERROR: spi_xfer() failed\n");
			break;
		}
	}
	return (rx[1] == 0xFF);
}

/*!
 * Sends a command to the radio chip and gets a response
 *
 * @param cmdByteCount  Number of bytes in the command to send to the radio device
 * @param pCmdData      Pointer to the command data
 * @param respByteCount Number of bytes in the response to fetch
 * @param pRespData     Pointer to where to put the response data
 *
 * @return CTS value
 */
uint8_t radio_comm_SendCmdGetResp(uint8_t cmdByteCount, uint8_t * pCmdData, uint8_t respByteCount, uint8_t * pRespData){
#ifdef RADIO_COMM_DEBUG
	printf("radio_comm_SendCmdGetResp()\n");
#endif
#ifdef LL_RADIO_COMM_DEBUG
	printArrHex(pCmdData, cmdByteCount);
#endif
    radio_comm_SendCmd(cmdByteCount, pCmdData);
    uint8_t rc = radio_comm_GetResp(respByteCount, pRespData);
#ifdef LL_RADIO_COMM_DEBUG
	printArrHex(pRespData, respByteCount);
#endif
	return rc;
}

