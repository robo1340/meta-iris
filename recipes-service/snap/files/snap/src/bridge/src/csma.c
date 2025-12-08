
#include <stddef.h>
#include "timer.h"
#include <czmq.h>

#define RANDINT(lower, upper) ((rand() % (upper-lower+1)) + lower)

/*
typedef enum {
    INIT,
	MEDIUM_BUSY,
	MEDIUM_BUSY_TX_PENDING,
	MEDIUM_IDLE,
	MEDIUM_IDLE_TX_PENDING
} csma_state_t;

typedef struct csma_DEF {
	csma_state_t state;
	uint8_t busy_threshold;
	uint8_t tx_probability; ///<the probability to transmit, a value from 0% to 100%
	uint16_t wait_upper;	///<upper limit in ms on the wait period
	uint16_t wait_lower; 	///<lower limit in ms on the wait period
	uint16_t period_ms;		///<current wait period in ms
	int64_t count_ms;		///<the current count in millis, -1 when timer not set
}csma_t;
*/

void csma_init(csma_t * csma, uint8_t busy_threshold, uint8_t tx_probability, uint16_t wait_lower, uint16_t wait_upper){
	csma->state = INIT;
	csma->busy_threshold = busy_threshold;
	csma->tx_probability = tx_probability;
	csma->wait_lower = wait_lower;
	csma->wait_upper = wait_upper;
	csma->period_ms = wait_lower;
	csma->count_ms = -1;
}

bool csma_medium_busy(csma_t * csma){
	return ((csma->state == MEDIUM_BUSY) || (csma->state == MEDIUM_BUSY_TX_PENDING));
}

bool csma_run(csma_t * csma){
	switch(csma->state){
		case INIT:
			break;
		case MEDIUM_BUSY:
			csma->state = MEDIUM_BUSY_TX_PENDING;
			break;
		case MEDIUM_BUSY_TX_PENDING:
			break;
		case MEDIUM_IDLE:
			return true;
			break;
		case MEDIUM_IDLE_TX_PENDING:
			if ((zclock_mono() - csma->count_ms) > csma->period_ms) {
				if (csma->tx_probability > RANDINT(0,100)){
					csma->state = MEDIUM_IDLE;
					return true; //should transmit now
				} else {
					csma->period_ms = RANDINT(csma->wait_lower, csma->wait_upper);
					csma->count_ms = zclock_mono();
				}
			}		
			break;	
		default: break;
	}
	return false;
}

bool csma_feed_rssi(csma_t * csma, uint32_t rssi){
	bool busy = (rssi >= csma->busy_threshold);
	
	switch(csma->state){
		case INIT:
		case MEDIUM_BUSY:
		case MEDIUM_IDLE:
			csma->state = (busy==true) ? MEDIUM_BUSY : MEDIUM_IDLE;
			break;
		case MEDIUM_BUSY_TX_PENDING:
			if (busy == false){
				csma->state = MEDIUM_IDLE_TX_PENDING;
				csma->period_ms = RANDINT(csma->wait_lower, csma->wait_upper);
				csma->count_ms = zclock_mono();
			}
			break;
		case MEDIUM_IDLE_TX_PENDING:
			if (busy == true){
				csma->state = MEDIUM_BUSY_TX_PENDING;
			}
			break;	
		default: break;
	}
	return busy;
}
