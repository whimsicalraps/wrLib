#pragma once

#include "wrBuffer.h"
#include "wrIPlayer.h"

typedef struct{
    buffer_t* buf;
    player_t* play;
} delay_t;

delay_t* delay_init( int samples );
void delay_deinit( delay_t* self );

void delay_rate( delay_t* self, float rate );
void delay_rate_v8( delay_t* self, float rate );
void delay_time( delay_t* self, float samples );
void delay_feedback( delay_t* self, float feedback );
void delay_length( delay_t* self, float fraction );
void delay_subloop( delay_t* self, bool is_subloop );
void delay_loop_to_here( delay_t* self, float length );
void delay_freeze( delay_t* self, bool is_freeze );

float delay_get_rate( delay_t* self );
float delay_get_time( delay_t* self );
float delay_get_feedback( delay_t* self );
float delay_get_length( delay_t* self );
bool delay_is_subloop( delay_t* self );
bool delay_is_freeze( delay_t* self );
float delay_get_cut( delay_t* self );

float delay_step( delay_t* self, float in );
float* delay_step_v( delay_t* self, float* io, int size );
