#pragma once

#include <stdint.h>

float tanh_fast(float in);
void tanh_fast_v(float* in, float* out, uint16_t size);
