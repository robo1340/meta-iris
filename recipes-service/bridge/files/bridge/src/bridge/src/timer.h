#ifndef _TIMER_H
#define _TIMER_H
#endif 

#include "state.h"

#include <stdint.h>
#include <stdbool.h>

void timer_init(task_timer_t * timer, bool enabled, uint32_t period_ms, uint32_t stachastic_ms);

bool timer_run(task_timer_t * timer);

bool timer_run_stochastic(task_timer_t * timer);

bool timer_reset(task_timer_t * timer);

bool timer_charge(task_timer_t * timer, uint32_t charge_ms);




