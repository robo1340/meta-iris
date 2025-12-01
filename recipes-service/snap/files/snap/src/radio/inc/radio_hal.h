/*!
 * File:
 *  radio_hal.h
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#ifndef _RADIO_HAL_H_
#define _RADIO_HAL_H_

#include <stdint.h>
#include <stdbool.h>

bool radio_hal_init(char * spidev_path);
bool radio_hal_deinit(void);

void radio_hal_AssertShutdown(void);
void radio_hal_DeassertShutdown(void);
//void radio_hal_ClearNsel(void);
//void radio_hal_SetNsel(void);
uint8_t radio_hal_NirqLevel(void);

void radio_hal_SpiWriteData(uint8_t byteCount, uint8_t * pData);
void radio_hal_SpiReadData(uint8_t byteCount, uint8_t * pData);

int radio_hal_transfer(uint8_t * tx, uint8_t * rx, size_t len);

#endif //_RADIO_HAL_H_
