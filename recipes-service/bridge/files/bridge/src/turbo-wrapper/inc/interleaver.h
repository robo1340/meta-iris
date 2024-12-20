#pragma once


#ifndef _TURBO_INTERLEAVE_
#define _TURBO_INTERLEAVE_

#include <stdint.h>

/* Interleaver for initialization - 8-bits */
void turbo_interleaver_init(void);
void turbo_interleaver_deinit(void);

/* Interleaver for initialization - 8-bits */
int turbo_interleave(int k, const uint8_t *input, uint8_t *output);

int turbo_deinterleave(int k, const uint8_t *in, uint8_t *out);

#endif /* _TURBO_INTERLEAVE_ */