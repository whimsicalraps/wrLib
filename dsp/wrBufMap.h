// a number of implementations of buf_map_t
// used for custom 'poke' functions that modify the buffer contents

#pragma once

#include "wrBuffer.h" // buf_map_t

// do-nothing map. testing. just use 'poke_v'
buf_map_t* buf_map_none_init( void );

// pre-level gain
buf_map_t* buf_map_gain_init( void );
void buf_map_gain_pre_level( buf_map_t* self, float c );

// lp1 filter & pre-level gain

#include "wrFilter.h"
buf_map_t* buf_map_filter_init( void );
void buf_map_filter_coeff( buf_map_t* self, float c );
void buf_map_filter_pre_level( buf_map_t* self, float c );
void buf_map_filter_set( buf_map_t* self, float o );
float buf_map_filter_get( buf_map_t* self );
