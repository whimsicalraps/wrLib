#pragma once

#include "wrBuffer.h"
#include "wrIHead.h"
#include "wrTransport.h"

typedef struct{
    buffer_t*     buf;
    ihead_fade_t* head;
    transport_t*  transport;

    float tape_end;
    bool  loop;
    float loop_start;
    float loop_end;
    float location; // 'playhead' pointer to buffer
    bool  going; // flag if a buf request is enqueued
    bool  play_before_erase;

    int   queued_location; // -1 == none
} player_t;

// setup
player_t* player_init( buffer_t* buffer );
void player_deinit( player_t* self );

// param setters
player_t* player_load( player_t* self, buffer_t* buffer );

void player_playing( player_t* self, bool is_play );
void player_goto( player_t* self, int sample );
void player_speed( player_t* self, float speed );
void player_speed_offset( player_t* self, float speed );
void player_nudge( player_t* self, int amount );
void player_recording( player_t* self, bool is_record );
void player_rec_level( player_t* self, float rec_level );
void player_pre_level( player_t* self, float pre_level );
void player_head_order( player_t* self, bool play_before_erase );
void player_loop( player_t* self, bool is_looping );
void player_loop_start( player_t* self, float location );
void player_loop_end( player_t* self, float location );

// param getters
bool player_is_playing( player_t* self );
float player_get_goto( player_t* self );
float player_get_speed( player_t* self );
float player_get_speed_offset( player_t* self );
float player_get_speed_live( player_t* self );
bool player_is_recording( player_t* self );
float player_get_rec_level( player_t* self );
float player_get_pre_level( player_t* self );
bool player_is_head_order( player_t* self );
bool player_is_looping( player_t* self );
float player_get_loop_start( player_t* self );
float player_get_loop_end( player_t* self );

// signals
float player_step( player_t* self, float in );
float* player_step_v( player_t* self, float* io, int size );
