/*!
 * File:
 *  radio_comm.h
 *
 * Description:
 *  This file contains the RADIO communication layer.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */
#ifndef _RADIO_COMM_H_
#define _RADIO_COMM_H_

#include "compiler_defs.h"
#include <stdint.h>
#include <stdbool.h>

#define RADIO_CTS_TIMEOUT 255
//#define RADIO_CTS_TIMEOUT 10000

                /* ======================================= *
                 *     G L O B A L   V A R I A B L E S     *
                 * ======================================= */
extern uint8_t radioCmd[16u];

//uint8_t radio_comm_GetResp(uint8_t byteCount, uint8_t * pData);
void radio_comm_SendCmd(uint8_t byteCount, uint8_t * pData);
void radio_comm_ReadData(uint8_t cmd, BIT pollCts, uint8_t byteCount, uint8_t * pData);
void radio_comm_WriteData(uint8_t cmd, BIT pollCts, uint8_t byteCount, uint8_t * pData);

bool radio_comm_PollCTS(void);
uint8_t radio_comm_SendCmdGetResp(uint8_t cmdByteCount, uint8_t * pCmdData, uint8_t respByteCount, uint8_t * pRespData);

#endif //_RADIO_COMM_H_
