#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdint.h>
#include <stdbool.h>

#include "compiler_defs.h"
#include "radio_config.h"
#include "si446x_api_lib.h"
#include "si446x_patch.h"
#include "si446x_cmd.h"
#include "radio_hal.h"
#include "radio_comm.h"

#define RADIO_DEFAULT_CHANNEL 0

bool radio_init(void);
bool radio_print_part_info(void);
bool radio_print_func_info(void);
char * radio_get_state_string(uint8_t state);
bool radio_print_state(bool print_if_no_change);
bool radio_print_modem_status(bool print_if_no_change);

uint8_t radio_get_modem_state(void);
uint8_t radio_get_modem_interrupt_pending(void);

bool radio_start_rx(uint8_t channel, size_t recv_len);

typedef struct {
    uint8_t   *Radio_ConfigurationArray;

    uint8_t   Radio_ChannelNumber;
    uint8_t   Radio_PacketLength;
    uint8_t   Radio_State_After_Power_Up;

    uint16_t  Radio_Delay_Cnt_After_Reset;

    //uint8_t   *Radio_Custom_Long_Payload;
} radio_configuration_t;

extern uint8_t Radio_Configuration_Data_Array[]; //from radio_config.h
extern radio_configuration_t RadioConfiguration; //from radio_config.h
extern radio_configuration_t * radio_config_ptr;

#endif 
