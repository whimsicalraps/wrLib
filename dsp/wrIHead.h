#pragma once

#include "wrBuffer.h"
#include "wrFilter.h"

#define OUT_BUF_LEN 64 // defines maximum speed

typedef struct{
    int   i;
    float f;
} phase_t;

typedef struct{
    // WRITE / ERASE HEAD
    float in_buf[4];
    int   in_buf_ix; // 'phase' but always advances by 1

    float out_buf[OUT_BUF_LEN];
    float out_phase;

    int write_ix; // pointer into the destination buffer

    bool  recording;

    float rec_level;
    float pre_level;

    // READ HEAD
    phase_t rphase;
} ihead_t;

////////////////////////////////////////
// setup
ihead_t* ihead_init( void );
void ihead_deinit( ihead_t* self );

////////////////////////////////////////
// params: setters
void ihead_jumpto( ihead_t* self, buffer_t* buf, int phase, bool is_forward );
void ihead_recording( ihead_t* self, bool is_recording );
void ihead_rec_level( ihead_t* self, float level );
void ihead_pre_level( ihead_t* self, float level );

////////////////////////////////////////
// params: getters
int ihead_get_location( ihead_t* self );
bool ihead_is_recording( ihead_t* self );
float ihead_get_rec_level( ihead_t* self );
float ihead_get_pre_level( ihead_t* self );

////////////////////////////////////////
// signal
// WRITE / ERASE HEAD
void ihead_poke( ihead_t*  self
               , buffer_t* buf
               , float     speed
               , float     input );
// READ HEAD
float ihead_peek( ihead_t* self, buffer_t* buf, float speed );
float* ihead_peek_v( ihead_t* self, float* io, buffer_t* buf, float* motion, int size );
