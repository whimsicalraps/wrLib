#pragma once

#include <stdint.h>
#include <wrFilter.h>

typedef struct{
    float sidechain;
    filter_lp1_a_t* lp1;
} compress_t;

compress_t* compress_init( void );
float* compress_step_v(compress_t* self, float* io, int size);
