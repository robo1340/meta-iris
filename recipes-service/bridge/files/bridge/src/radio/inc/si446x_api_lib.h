/*!
 * File:
 *  si446x_api_lib.h
 *
 * Description:
 *  This file contains the Si446x API library.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#ifndef _SI446X_API_LIB_H_
#define _SI446X_API_LIB_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "compiler_defs.h"
#include "si446x_cmd.h"
#include "si446x_nirq.h"
#include "si446x_prop.h"
#include "si446x_patch.h"
#include <czmq.h>

//#define SI466X_FIFO_SIZE 64
#define SI466X_FIFO_SIZE 129 //using combined fifo

extern union si446x_cmd_reply_union Si446xCmd;
extern uint8_t Pro2Cmd[16];

enum
{
    SI446X_SUCCESS,
    SI446X_NO_PATCH,
    SI446X_CTS_TIMEOUT,
    SI446X_PATCH_FAIL,
    SI446X_COMMAND_ERROR
};

/* Minimal driver support functions */
void si446x_reset(void);
void si446x_power_up(U8 BOOT_OPTIONS, U8 XTAL_OPTIONS, U32 XO_FREQ);

uint8_t si446x_configuration_init(const U8* pSetPropCmd);
bool si446x_configuration_init_enhanced(zhashx_t * radio_config, zlistx_t * si4463_load_order);
U8 si446x_apply_patch(void);
void si446x_part_info(void);

void si446x_start_tx(U8 CHANNEL, U8 CONDITION, U16 TX_LEN);
void si446x_start_rx(U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3);

void si446x_get_int_status(U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND);
void si446x_gpio_pin_cfg(U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG);

void si446x_set_property( U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... );
void si446x_change_state(U8 NEXT_STATE1);

/* Extended driver support functions */
void si446x_nop(void);

void si446x_fifo_info(U8 FIFO);
void si446x_write_tx_fifo( U8 numBytes, U8* pData );
void si446x_read_rx_fifo( U8 numBytes, U8* pRxData );

void si446x_get_property(U8 GROUP, U8 NUM_PROPS, U8 START_PROP);

/* Full driver support functions */
void si446x_func_info(void);

void si446x_frr_a_read(U8 respByteCount);
void si446x_frr_b_read(U8 respByteCount);
void si446x_frr_c_read(U8 respByteCount);
void si446x_frr_d_read(U8 respByteCount);

void si446x_get_adc_reading(U8 ADC_EN);
void si446x_get_packet_info(U8 FIELD_NUMBER_MASK, U16 LEN, S16 DIFF_LEN );
void si446x_get_ph_status(U8 PH_CLR_PEND);
void si446x_get_modem_status( U8 MODEM_CLR_PEND );
void si446x_get_chip_status( U8 CHIP_CLR_PEND );

void si446x_ircal_manual(U8 IRCAL_AMP, U8 IRCAL_PH);
void si446x_protocol_cfg(U8 PROTOCOL);

void si446x_request_device_state(void);

void si446x_tx_hop(U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0, U8 PLL_SETTLE_TIME1, U8 PLL_SETTLE_TIME0);
void si446x_rx_hop(U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0);

void si446x_start_tx_fast( void );
void si446x_start_rx_fast( void );

void si446x_get_int_status_fast_clear( void );
void si446x_get_int_status_fast_clear_read( void );

void si446x_gpio_pin_cfg_fast( void );

void si446x_get_ph_status_fast_clear( void );
void si446x_get_ph_status_fast_clear_read( void );

void si446x_get_modem_status_fast_clear( void );
void si446x_get_modem_status_fast_clear_read( void );

void si446x_get_chip_status_fast_clear( void );
void si446x_get_chip_status_fast_clear_read( void );

void si446x_fifo_info_fast_reset(U8 FIFO);
void si446x_fifo_info_fast_read(void);



#endif //_SI446X_API_LIB_H_
