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

bool radio_init(zhashx_t * radio_config, zlistx_t * si4463_load_order);
bool radio_print_part_info(void);
bool radio_print_func_info(void);
char * radio_get_state_string(uint8_t state);
bool radio_print_state(bool print_if_no_change);
bool radio_print_modem_status(bool print_if_no_change);

uint8_t radio_get_modem_state(void);
uint8_t radio_get_modem_interrupt_pending(void);

bool radio_start_rx(uint8_t channel, size_t recv_len);

#define RADIO_CONFIG_REGEX "^\\s*#define\\s+(RF_[^ ]*)\\s+(0x[^\\t\\r\\n]*)"
#define RADIO_MODEM_CONFIG_REGEX "^\\s*#define\\s+(RF_PA|RF_FREQ|RF_SYNTH|RF_M[^ ]*)\\s+(0x[^\\t\\r\\n]*)"

/*
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12 0x11, 0x21, 0x0C, 0x18, 0xB8, 0xDE, 0x05, 0x17, 0x16, 0x0C, 0x03, 0x00, 0x15, 0xFF, 0x00, 0x00
#define RF_PA_MODE_4 0x11, 0x22, 0x04, 0x00, 0x08, 0x7F, 0x00, 0x1D
#define RF_SYNTH_PFDCP_CPFF_7 0x11, 0x23, 0x07, 0x00, 0x2C, 0x0E, 0x0B, 0x04, 0x0C, 0x73, 0x03
#define RF_MATCH_VALUE_1_12 0x11, 0x30, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define RF_FREQ_CONTROL_INTE_8 0x11, 0x40, 0x08, 0x00, 0x3B, 0x08, 0x00, 0x00, 0x22, 0x22, 0x20, 0xFF
*/

/** @brief read in all radio configs in config_dir_path and all subdirectories
 *  and return a freshly allocated zhashx_t where keys are radio config keys and 
 *  values are zchunk_t describing the radio config options.
 *  Files in config_dir_path will be sorted, the first keys to be loaded will take precedence
 *  @param config_dir_path toplevel directory to search for radio config files
 *  @param current_config, the current config loaded, set to NULL if a fresh one is desired
 *  @return returns a zhashx_t*, caller responsible for destroying it when done
 */
zhashx_t * radio_load_configs(char * config_dir_path, zhashx_t * current_config, char * regex);

/** @brief read in the radio config at path and return a zhashx_t of key value pairs
 *  @param path path to the radio config
 *  @param current_config an existing zhashx_t containing radio config options, pass in NULL to create a new one
 *  @return returns a new zhashx_t
 */
zhashx_t * radio_parse_config(char * path, zhashx_t * current_config, char * regex);

#endif 
