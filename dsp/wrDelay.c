#include "wrDelay.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrBufferInterface.h"


//////////////////////////////////
// setup

delay_t* delay_init( int samples )
{
    delay_t* self = malloc( sizeof( delay_t ) );
    if( !self ){ printf("couldn't malloc delay_t.\n"); return NULL; }

    // objects
    self->buf = buffer_init( sizeof(float), samples, buffer_interface_init() );
    self->play = player_init( self->buf );

    delay_subloop( self, true ); // FIXME should be false
    player_playing( self->play, true );
    delay_freeze( self, false );
    player_rec_level( self->play, 1.0 );

    // params
    delay_rate( self, 1.0 );
    delay_feedback( self, 0.9 );

    return self;
}

void delay_deinit( delay_t* self )
{
    player_deinit( self->play );
    buffer_deinit( self->buf );
    free( self ); self = NULL;
}


////////////////////////////////////
// params

// setters
void delay_rate( delay_t* self, float rate )
{
    player_speed( self->play, rate );
}

#include <math.h>
void delay_rate_v8( delay_t* self, float rate )
{
    player_speed( self->play, powf( 2.0, rate ) );
}

// set buffer length at current rate, to match seconds
// if time is not acheivable at current rate, mul/div rate by 2 to acheive it
void delay_time( delay_t* self, float samples )
{
    float rate = delay_get_rate(self);
    float adjusted_s = samples * rate;
    if( adjusted_s <= 0.0 ){
        printf("delay_time zero or negative. set to max\n");
        player_loop( self->play, false );
    } else if( adjusted_s < 0.01 ){
        printf("TODO check delay_time not too short\n");
        //player_loop( self->play, true );
        //player_loop_start( self->play, start );
        //player_loop_end( self->play, start + bdiv );
    } else if( adjusted_s < self->play->tape_end ){ // FIXME account for LEAD_IN
        // apply length directly
        // FIXME rework after loop points can wrap over the buffer end
        player_loop_start( self->play, 0 );
        player_loop_end( self->play, adjusted_s );
        player_loop( self->play, true );
    } else {
        while( adjusted_s >= self->play->tape_end ){
            rate *= 0.5;
            adjusted_s = samples * rate;
        }
        if( rate >= (1.0/16.0) ){
            delay_rate( self, rate );
            player_loop_start( self->play, 0 );
            player_loop_end( self->play, adjusted_s );
            player_loop( self->play, true );
        } else {
            printf("TODO what should the minimum speed be?\n");
        }
    }
}

void delay_feedback( delay_t* self, float feedback )
{
    // TODO use log scaling for smoother feel & better decay finetuning
    // should this take loop time / rate into account?
    // ie shorter loop time == higher feedback for matched decay time
    player_pre_level( self->play, feedback );
}

// Set a sub-loop in the buffer which contains the current playhead
// Attempts to ensure the newest material stays in the loop brace
//  1. Sub-divide the total time into multiples of 'fraction'
//  2. Loop the 'fraction' that contains the current playhead
void delay_length( delay_t* self, float fraction )
{
    if( fraction >= 0.999 ){ // max time turns off looping
        player_loop( self->play, false );
    } else { player_loop( self->play, true ); }
    float bdiv = self->play->tape_end * fraction;
    int whole_divs = (int)(player_get_goto( self->play ) / bdiv);
    float start = (float)whole_divs * bdiv;

    player_loop_start( self->play, start );
    player_loop_end( self->play, start + bdiv );
}

void delay_subloop( delay_t* self, bool is_subloop )
{
    player_loop( self->play, is_subloop );
}

void delay_loop_to_here( delay_t* self, float length )
{
    float new_end = player_get_goto( self->play );
    float new_start = new_end - length;
    if( new_start < 0.0 ){
        printf("FIXME: wrap around loop boundaries\n");
        // for now, just pushing loop end point out
        new_end -= new_start; // push out loop end (new_start is negative)
        new_start = 0.0;
    }
    player_loop_start( self->play, new_start );
    player_loop_end( self->play, new_end );
    player_loop( self->play, true );
}

void delay_freeze( delay_t* self, bool is_freeze )
{
    player_recording( self->play, !is_freeze );
}

// getters
float delay_get_rate( delay_t* self )
{
    return player_get_speed( self->play );
}

float delay_get_time( delay_t* self )
{
    float t = 0.0;
    if( player_is_looping( self->play ) ){
        // FIXME handle outside loops (ie end before start)
        t = player_get_loop_end( self->play ) - player_get_loop_start( self->play );
    } else {
        t = self->play->tape_end;
    }
    return t / delay_get_rate( self );
}

float delay_get_length( delay_t* self )
{
    return delay_get_time( self ) / self->play->tape_end;
}

float delay_get_feedback( delay_t* self )
{
    return player_get_pre_level( self->play );
}

bool delay_is_subloop( delay_t* self )
{
    return player_is_looping( self->play );
}

bool delay_is_freeze( delay_t* self )
{
    return !player_is_recording( self->play );
}

float delay_get_cut( delay_t* self )
{
    return player_get_goto( self->play );
}


///////////////////////////////////
// signal

float delay_step( delay_t* self, float in )
{
    return player_step( self->play, in );
}

float* delay_step_v( delay_t* self, float* io, int size )
{
    return player_step_v( self->play, io, size );
}
