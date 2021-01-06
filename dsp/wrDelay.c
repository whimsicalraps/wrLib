#include "wrDelay.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrBufferInterface.h"

/////////////////////////////////////
// private declarations

static void apply_rate( delay_t* self );


//////////////////////////////////
// setup

delay_t* delay_init( Buf_Type_t type, int samples )
{
    delay_t* self = malloc( sizeof( delay_t ) );
    if( !self ){ printf("couldn't malloc delay_t.\n"); return NULL; }

    // objects
    buffer_interface_t* bi = buffer_interface_init(type); // generic buffer i.f

    bi->userdata = (void*)self; // give mac_v access to this object
    switch( type ){
        case Buf_Type_Float:
            self->buf = buffer_init( sizeof(float), samples, bi );
            break;
        case Buf_Type_S16:
            self->buf = buffer_init( sizeof(int16_t), samples, bi );
            break;
        default: printf("delay: buffer type unsupported.\n"); return NULL;
    }
    // causes some noises at loop bounds i think
    // still sounds freaking great though!
    self->play = player_init( self->buf, Buf_Map_Filter );

    delay_lowpass( self, 0.0 );
    delay_subloop( self, false ); // FIXME should be false
    player_playing( self->play, true );
    delay_freeze( self, false );
    player_rec_level( self->play, 1.0 );
    player_head_order( self->play, true );

    // params
    delay_rate( self, 1.0 );
    delay_feedback( self, 0.8 );

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
    self->play->transport->speeds.accel_standard = 0.01;
    self->rate = rate;
    apply_rate( self );
}

void delay_rate_smoothed( delay_t* self, float rate )
{
    // FIXME add smoothing
    self->play->transport->speeds.accel_standard = 0.002;
    self->rate = rate;
    apply_rate( self );
}

#include <math.h>
void delay_rate_v8( delay_t* self, float rate )
{
    self->rate = exp2f( rate );
    apply_rate( self );
}

void delay_rate_mod( delay_t* self, float mod )
{
    // TODO use player_nudge to support mod natively
    self->mod = mod; // TODO bounds?
    apply_rate( self );
}

#define DELAY_SAMPLERATE    ((float)48000.0) // FIXME!
#define C3_HZ               ((float)261.63)  // hz
#define C3_TIME             (DELAY_SAMPLERATE/C3_HZ) // samples

// set buffer length at current rate, to match seconds
// if time is not acheivable at current rate, mul/div rate by 2 to acheive it
void delay_time( delay_t* self, float samples )
{
    double rate = delay_get_rate(self);
    phase_t adjusted_s = phase_mul_d( phase_from_double(samples)
                                    , rate );
    if( phase_lt( adjusted_s, phase_zero() ) ){
        player_loop( self->play, 0 );
    } else if( phase_ez( adjusted_s ) ){
        player_loop_start( self->play, phase_from_double(1000.0) ); // avoid LEADIN area
        player_loop_end( self->play, phase_from_double(1000.0 + C3_TIME) );
        player_loop( self->play, 1 );
    } else if( phase_lt( adjusted_s
                       , player_get_tape_length( self->play ) ) ){
        delay_loop_to_here( self, adjusted_s );
    } else { // longer than 1x capable
        // half samples & rate until in range
        phase_t tape_len = player_get_tape_length( self->play );
        while( phase_gte( adjusted_s, tape_len ) ){
            rate *= (double)0.5;
            adjusted_s = phase_mul_d( adjusted_s, 0.5 );
        }
        if( rate >= (double)(1.0/16.0) ){
            delay_rate( self, rate );
            delay_loop_to_here( self, adjusted_s );
        } else {
            printf("TODO ignoring delay_time as rate would be <(1/16).\n");
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
// void delay_length( delay_t* self, float fraction )
// {
//     if( fraction >= 0.999 ){ // max time turns off looping
//         player_loop( self->play, 0 );
//         return;
//     }
//     float new_len = fraction * player_get_tape_length( self->play );
//     delay_loop_to_here( self, new_len );
// }

// multiply the current loop length by *mul*
void delay_length_mul( delay_t* self, float mul )
{
    if( !player_get_looping( self->play ) ){
        if( mul < 1.0 ){
            // activate loop & set as a fraction of the whole
            phase_t tape_len = player_get_tape_length( self->play );
            phase_t new_len = phase_mul_d( tape_len, mul );
            delay_loop_to_here( self, new_len );
        }
    } else {
        phase_t loop_len = player_get_loop_size( self->play );
        phase_t new_len = phase_mul_d( loop_len, mul );
        if( phase_gte( new_len, player_get_tape_length(self->play) ) ){
            printf("loop_mul goes past 1.0\n");
            player_loop( self->play, 0 ); // loop longer than available time
        } else {
            delay_loop_to_here( self, new_len );
        }
    }
}

void delay_subloop( delay_t* self, int subloop )
{
    player_loop( self->play, subloop );
}

void delay_loop_to_here( delay_t* self, phase_t length )
{
    if( phase_lt( length, phase_new(LOOP_MIN_LENGTH,0.0) ) ){ return; }
    phase_t new_end = player_get_goto( self->play );
    phase_t new_start = phase_sub( new_end, length );

    switch( player_is_location_off_tape( self->play, new_start ) ){
        case  1: new_start = phase_sub( new_start
                                , player_get_tape_length( self->play ) ); break;
        case -1: new_start = phase_add( new_start
                                , player_get_tape_length( self->play ) ); break;
        default: break;
    }
    player_loop_start( self->play, new_start );
    player_loop_end( self->play, new_end );
    player_loop( self->play, 1 );
}

void delay_freeze( delay_t* self, bool is_freeze )
{
    player_recording( self->play, !is_freeze );
}

void delay_lowpass( delay_t* self, float coeff )
{
    self->lpf = (coeff < 0.0) ? 0.0 : (coeff > 1.0) ? coeff = 1.0 : coeff;
    player_pre_filter( self->play, self->lpf );
}

// ratiometric loop+cut modifiers
void delay_ratio_length( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero
    if( n==d ){ player_loop(self->play, 0); return; } // full size
    phase_t tape_len = player_get_tape_length( self->play );
    phase_t loop_len = phase_mul_d( tape_len, (float)n / (float)d );
    // push out the loop_end to match
    phase_t new_end = phase_add( player_get_loop_start( self->play )
                               , loop_len );
    switch( player_is_location_off_tape( self->play, new_end ) ){
        case  1: new_end = phase_sub( new_end
                            , player_get_tape_length( self->play ) ); break;
        case -1: new_end = phase_add( new_end
                            , player_get_tape_length( self->play ) ); break;
        default: break;
    }
    player_loop_end( self->play, new_end );
    player_loop( self->play, 1 );
}
void delay_ratio_position( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero
    //phase_t tape_len = phase_new( player_get_tape_length( self->play ), 0.0);
    phase_t tape_len = player_get_tape_length( self->play );
    phase_t segment = phase_mul_d( tape_len, (double)1.0/(double)d );
    phase_t position = phase_mul_d( segment, (double)n ); // new start location
    phase_t lsize = player_get_loop_size( self->play );

    phase_t new_end = phase_add( position, lsize );
    switch( player_is_location_off_tape( self->play, new_end ) ){
        case  1: new_end = phase_sub( new_end
                            , player_get_tape_length( self->play ) ); break;
        case -1: new_end = phase_add( new_end
                            , player_get_tape_length( self->play ) ); break;
        default: break;
    }
    player_loop_start( self->play, position );
    player_loop_end( self->play, new_end );
}
void delay_ratio_cut( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero

    if( player_get_looping( self->play ) ){ // slice up the loop
        phase_t lsize = player_get_loop_size( self->play );
        phase_t cut = phase_mul_d( lsize, (double)n/(double)d );
        phase_t start = player_get_loop_start( self->play );
        phase_t dest_cut = phase_sub( cut, start );
        if( phase_gt( dest_cut, player_get_tape_length(self->play) ) ){ // wraps end of buffer
            phase_t rev_cut = phase_add( lsize, phase_sub( cut, phase_new(1,0.0)));
            phase_t end = player_get_loop_end( self->play );
            player_goto( self->play, phase_sub( end, rev_cut ) );
        } else { // plain loop
            player_goto( self->play, dest_cut );
        }
    } else { // slice up the whole buffer
        phase_t tape_len = player_get_tape_length( self->play );
        player_goto( self->play, phase_mul_d( tape_len, (double)n/(double)d ) );
    }
}



//////////////////////////////////////////////
// getters

float delay_get_rate( delay_t* self )
{
    return self->rate;
}

float delay_get_time( delay_t* self )
{
    return player_get_loop_size_int(self->play) / delay_get_rate( self );
}

float delay_get_length( delay_t* self )
{
    return (float)player_get_loop_size_int(self->play) / (float)player_get_tape_length_int( self->play);
}

float delay_get_feedback( delay_t* self )
{
    return player_get_pre_level( self->play );
}

int delay_is_subloop( delay_t* self )
{
    return player_get_looping( self->play );
}

bool delay_is_freeze( delay_t* self )
{
    return !player_is_recording( self->play );
}

float delay_get_cut( delay_t* self )
{
    return player_get_goto( self->play ).i;
}

float delay_get_lowpass( delay_t* self )
{
    return self->lpf;
}

bool delay_is_near_loop( delay_t* self )
{
    const int window = 850; // ensures visability at 2x speed
    int now = player_get_goto( self->play ).i;
    int start = player_get_loop_start( self->play ).i;

    if( now < (start+window)
     && now > (start-window) ){ return true; }
    else{ return false; }
}


///////////////////////////////////
// signal

float delay_step( delay_t* self, float in )
{
    return player_step( self->play, in );
}

float* delay_step_v( delay_t* self, float* io, int size )
{
// uncomment to use block processing version
        // pro: less than half the CPU usage
        // con: delay time can't preceed 'size'
    //return player_step_v( self->play, io, size );

    // using single-sample version to force sample accuracy
    float* b = io;
    for( int i=0; i<size; i++ ){
        *b = player_step( self->play, *b );
        b++;
    }
    return io;
}


//////////////////////////////////////
// private definitions

static void apply_rate( delay_t* self )
{
    player_speed( self->play, self->rate + self->mod );
}
