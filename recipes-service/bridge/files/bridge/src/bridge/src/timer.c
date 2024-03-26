
#include <stddef.h>
#include "timer.h"
#include <czmq.h>

#define RANDINT(lower, upper) ((rand() % (upper-lower+1)) + lower)

void timer_init(task_timer_t * timer, bool enabled, uint32_t period_ms, uint32_t stachastic_ms){
	timer->enabled = enabled;
	timer->period_ms = period_ms;
	timer->count_ms = 0;
	timer->stachastic_ms = stachastic_ms;
}

bool timer_run(task_timer_t * timer){
	if ((zclock_mono() - timer->count_ms) > timer->period_ms){
		if (timer->stachastic_ms==0){
			timer->count_ms = zclock_mono();
		} else {
			timer->count_ms = zclock_mono() + RANDINT((int)((~timer->stachastic_ms)+1),timer->stachastic_ms);
		}
		return true;
	}
	return false;
}

bool timer_reset(task_timer_t * timer){
	timer->count_ms = zclock_mono();
	return true;
}

bool timer_charge(task_timer_t * timer, uint32_t charge_ms){
	timer->count_ms = (zclock_mono() - timer->period_ms) + charge_ms;
	return true;
}

