#pragma once

#include "wrIHead.h"
//#include "wrBuffer.h"
//#include "wrFilter.h"
#include "wrBufMap.h" // buf_map_t

//#define OUT_BUF_LEN 64 // defines maximum speed

typedef struct{
    // public
    float    fade_length;
    float    fade_rec_level;
    float    fade_pre_level;
    float    fade_pre_filter;

    // private
    ihead_t* head[2];
    int      fade_active_head;
    float    fade_phase;
    float    fade_increment; // 0-1 normalized per-sample
    int      fade_countdown;
    filter_lp1_t* rec_slew;
    filter_lp1_t* pre_slew;
    bool     fade_recording_dest; // recording state destination
} ihead_fade_t;


////////////////////////////////////////
// setup
ihead_fade_t* ihead_fade_init( Buf_Map_Type_t map_type );
void ihead_fade_deinit( ihead_fade_t* self );

////////////////////////////////////////
// params: setters
void ihead_fade_jumpto( ihead_fade_t* self, buffer_t* buf, phase_t phase, bool is_forward );
void ihead_fade_align( ihead_fade_t* self, bool is_forward );
void ihead_fade_recording( ihead_fade_t* self, bool is_recording );
void ihead_fade_xfade_ms( ihead_fade_t* self, float time );
void ihead_fade_rec_level( ihead_fade_t* self, float level );
void ihead_fade_pre_level( ihead_fade_t* self, float level );
void ihead_fade_pre_filter( ihead_fade_t* self, float coeff );

////////////////////////////////////////
// params: getters
phase_t ihead_fade_get_location( ihead_fade_t* self );
bool ihead_fade_is_recording( ihead_fade_t* self );
float ihead_fade_get_rec_level( ihead_fade_t* self );
float ihead_fade_get_pre_level( ihead_fade_t* self );

////////////////////////////////////////
// signal
// WRITE / ERASE HEAD
void ihead_fade_poke( ihead_fade_t*  self
                    , buffer_t*      buf
                    , float          speed
                    , float          input
                    );
void ihead_fade_poke_v( ihead_fade_t*  self
                      , buffer_t*      buf
                      , float*         motion
                      , float*         io
                      , int            size
                      );

// READ HEAD
float ihead_fade_peek( ihead_fade_t* self, buffer_t* buf, float speed );
float* ihead_fade_peek_v( ihead_fade_t* self
                        , float*        io
                        , buffer_t*     buf
                        , float*        motion
                        , int           size );
