#pragma once

#include "wrBuffer.h"
#include "wrIPlayer.h"

typedef struct{
    buffer_t* buf;
    player_t* play;
    float     rate; // base rate, to apply modulation on top of
    float     mod; // rate modulation amount
    float     lpf;
} delay_t;

delay_t* delay_init( int samples );
void delay_deinit( delay_t* self );

void delay_rate( delay_t* self, float rate ); // set delay sample-rate re: input SR
void delay_rate_smoothed( delay_t* self, float rate ); // as above with slewed change
void delay_rate_v8( delay_t* self, float rate ); // as above with 2^rate scaling
void delay_rate_mod( delay_t* self, float mod ); // add a linear offset to rate
void delay_time( delay_t* self, float samples ); // set rate-relative-time in samples
void delay_feedback( delay_t* self, float feedback );
void delay_length( delay_t* self, float fraction ); // set subloop to a fraction of buffer time. ignores rate. captures the current playhead.
void delay_subloop( delay_t* self, int subloop ); // activates the subloop
void delay_loop_to_here( delay_t* self, float length ); // set current playhead as loop end, and loop the previous length of samples
void delay_freeze( delay_t* self, bool is_freeze ); // disable recording & erase
void delay_lowpass( delay_t* self, float coeff );

float delay_get_rate( delay_t* self ); // (delay SR / playback SR)
float delay_get_time( delay_t* self ); // time, adjusted for rate, in samples
float delay_get_feedback( delay_t* self ); // (0,1)
float delay_get_length( delay_t* self ); // portion of buffer currently active (0,1)
int delay_is_subloop( delay_t* self ); // playback is restricted to a sub-loop
bool delay_is_freeze( delay_t* self ); // recording (and erase) is disabled
float delay_get_cut( delay_t* self ); // current playhead location in samples
float delay_get_lowpass( delay_t* self );

float delay_step( delay_t* self, float in );
float* delay_step_v( delay_t* self, float* io, int size );
