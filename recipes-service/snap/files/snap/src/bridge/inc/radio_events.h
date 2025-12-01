#ifndef radio_events_H_
#define radio_events_H_

#include <stdint.h>
#include <stdbool.h>
#include "state.h"

bool radio_sync_detect_callback(state_t * state);

bool radio_receive_pending_callback(state_t * state);

bool radio_receive_fifo_almost_full_callback(state_t * state, uint32_t rx_almost_full_threshhold);

bool radio_transmit_complete_callback(state_t * state);

bool radio_transmit_fifo_almost_full_callback(state_t * state, uint32_t tx_almost_full_treshhold);

/** @brief call when an interrupt from the radio has been received
 *  @param state the state of the program
 *  @return returns false on error
 */
bool radio_event_callback(state_t * state);

#endif
