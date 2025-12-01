
#include "avg.h"

uint32_t exp_moving_avg(uint32_t new_sample){
	static float alpha = 0.7;
	static float alpha_m1 = 0.3;
	static float avg = 0;
	
	float new_sample_f = (float)new_sample;
	avg = (alpha*new_sample_f) + (alpha_m1*avg);
	return (uint32_t)avg;
}
