/*!
 * File:
 *  radio_hal.c
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "spidev.h"
#include "gpio.h"

#include "radio_hal.h"

int si4463_spi_fd = 0;

#define SPI_SPEED 10000000 //10Mbps

bool radio_hal_init(char * spidev_path){
	spi_config_t conf = {.mode=0, .bits_per_word=8, .speed=SPI_SPEED, .delay=0};
	si4463_spi_fd = spi_open(spidev_path, conf);
	if (si4463_spi_fd < 0) {return false;}
	return true;
}

bool radio_hal_deinit(void){
	spi_close(si4463_spi_fd);
	si4463_spi_fd = 0;
	return true;
}


void radio_hal_AssertShutdown(void){
//////////////////////////////////
	//libgpio command here
//////////////////////////////////
}

void radio_hal_DeassertShutdown(void){
//////////////////////////////////
	//libgpio command here
//////////////////////////////////
}

//void radio_hal_ClearNsel(void){}
//void radio_hal_SetNsel(void){}

uint8_t radio_hal_NirqLevel(void){
	return gpio_get() ? 0 : 1;
}

void radio_hal_SpiWriteData(uint8_t byteCount, uint8_t* pData){
	spi_write(si4463_spi_fd, pData, byteCount);
}

void radio_hal_SpiReadData(uint8_t byteCount, uint8_t* pData){
	spi_read(si4463_spi_fd, pData, byteCount);
}

int radio_hal_transfer(uint8_t * tx, uint8_t * rx, size_t len){
	return spi_xfer(si4463_spi_fd, tx, len, rx, len);
}
