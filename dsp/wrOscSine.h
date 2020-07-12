#pragma once

#include <stdint.h>

#define LUT_SIN_SIZE  ((uint32_t)1024)
#define LUT_SIN_HALF  (LUT_SIN_SIZE >> 1)
extern const float sine_lut[];

typedef struct osc_sine{
    float rate;
    float id;
} osc_sine_t;

// initialization
osc_sine_t* sine_init( void );
void sine_deinit( osc_sine_t* self );

// input fns
    // where time = 0, oscillator stopped
    //       time = 1, oscillator at sample rate
void osc_sine_time( osc_sine_t* self, float time );
void osc_sine_reset( osc_sine_t* self );

// process
float osc_sine_step( osc_sine_t* self, float fm ); // fm == 0 is no mod
void osc_sine_process_v( osc_sine_t* self
                       , uint16_t    b_size
                       , float*      exp_fm // expo scaler (*)
                       , float*      lin_fm // linear offset (+)
                       , float*      out );
float* sine_process_base_v( osc_sine_t* self
                          , float*      out
                          , int    b_size );
