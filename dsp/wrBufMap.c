#include "wrBufMap.h"

#include <stdio.h>
#include <stdlib.h>

#include "wrFilter.h" // used by buf_map_filter

//////////////////////////////////////////////////
// do-nothing map. testing. just use 'poke_v'

static float buf_map_none( void* userdata, float input ){
    return input;
}

buf_map_t* buf_map_none_init( void )
{
    buf_map_t* self = malloc( sizeof(buf_map_t) );
    if( !self ){ printf("malloc failed\n"); return NULL; }
    self->fn = &buf_map_none;
    self->type = Buf_Map_None;
    self->userdata = NULL;
    return self;
}


////////////////////////////////////
// pre-level gain

typedef struct{
    float pre_level;
} buf_map_gain_t;

static float buf_map_gain( void* userdata, float input )
{
    buf_map_gain_t* ud = (buf_map_gain_t*)userdata;
    return input * ud->pre_level;
}

buf_map_t* buf_map_gain_init( void )
{
    buf_map_t* self = malloc( sizeof(buf_map_t) );
    if( !self ){ printf("malloc failed 1\n"); return NULL; }
    self->fn = &buf_map_gain;
    self->type = Buf_Map_Gain;
    self->userdata = malloc( sizeof(buf_map_gain_t) );
    if( !self->userdata ){ printf("malloc failed 2\n"); return NULL; }
    buf_map_gain_t* ud = self->userdata;
    ud->pre_level = 0.0;
    return self;
}

void buf_map_gain_pre_level( buf_map_t* self, float c ){
    buf_map_gain_t* g = (buf_map_gain_t*)self->userdata;
    g->pre_level = c;
}


//////////////////////////////////////////
// lp1 filter & pre-level gain

typedef struct{
    float         pre_level;
    int           next;
    filter_lp1_t* filter;
} buf_map_filter_t;

static float buf_map_filter( void* userdata, float input )
{
    buf_map_filter_t* f = (buf_map_filter_t*)userdata;
    return lp1_step(f->filter, input) * f->pre_level;
}

buf_map_t* buf_map_filter_init( void )
{
    buf_map_t* self = malloc( sizeof(buf_map_t) );
    if( !self ){ printf("malloc failed 1\n"); return NULL; }
    self->fn = &buf_map_filter;
    self->type = Buf_Map_Filter;
    self->userdata = malloc( sizeof(buf_map_filter_t) );
    if( !self->userdata ){ printf("malloc failed 2\n"); return NULL; }
    buf_map_filter_t* ud = self->userdata;
    ud->pre_level = 0.0;
    ud->filter = lp1_init();
    buf_map_filter_coeff(self, 0.1);
    if( !ud->filter ){ printf("malloc failed 3\n"); return NULL; }
    return self;
}
void buf_map_filter_coeff( buf_map_t* self, float c ){
    buf_map_filter_t* f = (buf_map_filter_t*)self->userdata;
    lp1_set_coeff(f->filter, c);
}
void buf_map_filter_pre_level( buf_map_t* self, float c ){
    buf_map_filter_t* f = (buf_map_filter_t*)self->userdata;
    f->pre_level = c;
}
void buf_map_filter_set( buf_map_t* self, float o ){
    buf_map_filter_t* f = (buf_map_filter_t*)self->userdata;
    lp1_set_out(f->filter, o);
}
float buf_map_filter_get( buf_map_t* self ){
    buf_map_filter_t* f = (buf_map_filter_t*)self->userdata;
    return lp1_get_out( f->filter );
}
