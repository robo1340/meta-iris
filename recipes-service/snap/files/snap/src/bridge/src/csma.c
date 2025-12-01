
#include <stddef.h>
#include "timer.h"
#include <czmq.h>

#define RANDINT(lower, upper) ((rand() % (upper-lower+1)) + lower)

/*
typedef struct csma_DEF {
	bool enabled; 			///<csma is active
	uint8_t tx_probability; ///<the probability to transmit, a value from 0% to 100%
	uint16_t wait_upper;	///<upper limit in ms on the wait period
	uint16_t wait_lower; 	///<lower limit in ms on the wait period
	uint16_t period_ms;		///<current wait period in ms
	int64_t count_ms;		///<the current count in millis, -1 when timer not set
}csma_t;
*/

void csma_init(csma_t * csma, uint8_t tx_probability, uint16_t wait_lower, uint16_t wait_upper){
	csma->enabled = false;
	csma->tx_probability = tx_probability;
	csma->wait_lower = wait_lower;
	csma->wait_upper = wait_upper;
	csma->period_ms = wait_lower;
	csma->count_ms = -1;
}

bool csma_enabled(csma_t * csma){
	return csma->enabled;
}

//enable the csma algorithm
void csma_reset(csma_t * csma){
	csma->enabled = true;
	csma->period_ms = RANDINT(csma->wait_lower, csma->wait_upper);
	csma->count_ms = zclock_mono();
}

//disable the csma algorithm
void csma_disable(csma_t * csma){
	csma->enabled = false;
	csma->count_ms = -1;
}

//returns true if countdown ended
bool csma_run_countdown(csma_t * csma){
	if ((zclock_mono() - csma->count_ms) > csma->period_ms) {
		csma_reset(csma);
		return true;
	}
	return false;
}

//returns true if tx_probability threshhold was reached
bool csma_run_transmit(csma_t * csma){
	if (csma->tx_probability > RANDINT(0,100)){
		csma_disable(csma);
		return true; //should transmit now
	} else {
		csma_reset(csma);
		return false;
	}
}
