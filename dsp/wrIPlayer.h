#pragma once

#include "wrBuffer.h"
#include "wrIHeadFade.h" // phase_t
#include "wrTransport.h"

typedef struct{
    buffer_t*     buf;
    ihead_fade_t* head;
    transport_t*  transport;

    phase_t tape_end;        // end of the *buffer
    phase_t tape_end_lead;   // accessible end (loop here)
    phase_t tape_start_lead; // start after lead-in (loop here)
    phase_t tape_size;       // accessible length (-2*leadin)
    int     loop;
    phase_t loop_start;
    phase_t loop_end;
    phase_t loop_size; // memoized for speed
    phase_t o_loop_start; // ordered version of above
    phase_t o_loop_end; // ordered version of above
    bool    going; // flag if a buf request is enqueued
    bool    play_before_erase;

    phase_t queued_location; // -1 == none
} player_t;

// setup
player_t* player_init( buffer_t* buffer );
void player_deinit( player_t* self );

// param setters
player_t* player_load( player_t* self, buffer_t* buffer );

void player_playing( player_t* self, bool is_play );
void player_goto( player_t* self, phase_t sample );
void player_speed( player_t* self, float speed );
void player_speed_offset( player_t* self, float speed );
void player_nudge( player_t* self, int amount );
void player_recording( player_t* self, bool is_record );
void player_rec_level( player_t* self, float rec_level );
void player_pre_level( player_t* self, float pre_level );
void player_head_order( player_t* self, bool play_before_erase );
void player_loop( player_t* self, int looping );
        void player_loop_start_int( player_t* self, int location );
        void player_loop_end_int( player_t* self, int location );
void player_loop_start( player_t* self, phase_t location );
void player_loop_end( player_t* self, phase_t location );

// param getters
bool player_is_playing( player_t* self );
phase_t player_get_goto( player_t* self );
phase_t player_get_queued_goto( player_t* self );
float player_get_speed( player_t* self );
float player_get_speed_offset( player_t* self );
float player_get_speed_live( player_t* self );
bool player_is_recording( player_t* self );
float player_get_rec_level( player_t* self );
float player_get_pre_level( player_t* self );
bool player_is_head_order( player_t* self );
int player_get_looping( player_t* self );
        int player_get_loop_start_int( player_t* self );
        int player_get_loop_end_int( player_t* self );
phase_t player_get_loop_start( player_t* self );
phase_t player_get_loop_end( player_t* self );
        int player_get_loop_size_int( player_t* self );
phase_t player_get_loop_size( player_t* self );
int player_get_tape_length_int( player_t* self );
phase_t player_get_tape_length( player_t* self );

// queries
bool player_is_location_in_loop( player_t* self, phase_t location );
float player_position_in_loop( player_t* self, int location );
int player_is_location_off_tape( player_t* self, phase_t location );

// signals
float player_step( player_t* self, float in );
float* player_step_v( player_t* self, float* io, int size );
