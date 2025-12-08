#ifndef _CSMA_H
#define _CSMA_H
#endif 

#include "state.h"

#include <stdint.h>
#include <stdbool.h>

void csma_init(csma_t * csma, uint8_t busy_threshold, uint8_t tx_probability, uint16_t wait_lower, uint16_t wait_upper);

bool csma_medium_busy(csma_t * csma);

bool csma_run(csma_t * csma);

bool csma_feed_rssi(csma_t * csma, uint32_t rssi);




