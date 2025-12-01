#ifndef _CSMA_H
#define _CSMA_H
#endif 

#include "state.h"

#include <stdint.h>
#include <stdbool.h>

void csma_init(csma_t * csma, uint8_t tx_probability, uint16_t wait_lower, uint16_t wait_upper);

bool csma_enabled(csma_t * csma);

//enable the csma algorithm
void csma_reset(csma_t * csma);

//disable the csma algorithm
void csma_disable(csma_t * csma);

//returns true if countdown ended
bool csma_run_countdown(csma_t * csma);

//returns true if tx_probability threshhold was reached
bool csma_run_transmit(csma_t * csma);




