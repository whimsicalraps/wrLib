#include "wrDelay.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrBufferInterface.h"

static void apply_rate( delay_t* self );
static void mac_v( buffer_interface_t* self, float* io
                                    , int origin
                                    , int count
                                    , float coeff);


//////////////////////////////////
// setup

delay_t* delay_init( int samples )
{
    delay_t* self = malloc( sizeof( delay_t ) );
    if( !self ){ printf("couldn't malloc delay_t.\n"); return NULL; }

    // objects
    buffer_interface_t* bi = buffer_interface_init(); // generic buffer i.f
    bi->mac_v = &mac_v; // overload with our LPfiltering mac function
    bi->userdata = (void*)self; // give mac_v access to this object
    self->buf = buffer_init( sizeof(float), samples, bi );
    self->play = player_init( self->buf );
    self->lpf = 0.0;

    delay_subloop( self, false ); // FIXME should be false
    player_playing( self->play, true );
    delay_freeze( self, false );
    player_rec_level( self->play, 1.0 );

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
    self->play->transport->speeds.accel_standard = 0.00005;
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
    float rate = delay_get_rate(self);
    float adjusted_s = samples * rate;
    if( adjusted_s < 0.0 ){
        player_loop( self->play, 0 );
    } else if( adjusted_s == 0.0 ){
        player_loop_start( self->play, 1000 ); // avoid LEADIN area
        player_loop_end( self->play, 1000 + C3_TIME );
        player_loop( self->play, 1 );
    } else if( adjusted_s < player_get_tape_length( self->play ) ){
        // apply length from 'here'
        delay_loop_to_here( self, adjusted_s );
    } else { // longer than 1x capable
        // half samples & rate until in range
        float tape_len = player_get_tape_length( self->play );
        while( adjusted_s >= tape_len ){
            rate *= 0.5;
            adjusted_s = samples * rate;
        }
        if( rate >= (1.0/16.0) ){
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
void delay_length( delay_t* self, float fraction )
{
    if( fraction >= 0.999 ){ // max time turns off looping
        player_loop( self->play, 0 );
        return;
    }
    float new_len = fraction * player_get_tape_length( self->play );
    delay_loop_to_here( self, new_len );
}

void delay_subloop( delay_t* self, int subloop )
{
    player_loop( self->play, subloop );
}

void delay_loop_to_here( delay_t* self, float length )
{
    if( length < LOOP_MIN_LENGTH ){ return; }
    float new_end = player_get_goto( self->play );
    float new_start = new_end - length;
    switch( player_is_location_off_tape( self->play, new_start ) ){
        case 1: new_start -= player_get_tape_length( self->play ); break;
        case -1: new_start += player_get_tape_length( self->play ); break;
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
}

// ratiometric loop+cut modifiers
void delay_ratio_length( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero
    if( n==d ){ player_loop(self->play, 0); return; } // full size
    float tape_len = player_get_tape_length( self->play );
    float loop_len = tape_len * (float)n / (float)d;
    // push out the loop_end to match
    float new_end = player_get_loop_start( self->play ) + loop_len;
    switch( player_is_location_off_tape( self->play, new_end ) ){
        case 1: new_end -= player_get_tape_length( self->play ); break;
        case -1: new_end += player_get_tape_length( self->play ); break;
        default: break;
    }
    player_loop_end( self->play, new_end );
    player_loop( self->play, 1 );
}
void delay_ratio_position( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero
    double tape_len = player_get_tape_length( self->play );
    double segment  = tape_len / (double)d;
    double position = segment * (double)n; // new start location
    double lsize = player_get_loop_size( self->play );

    double new_end = position + lsize;
    switch( player_is_location_off_tape( self->play, new_end ) ){
        case 1: new_end -= player_get_tape_length( self->play ); break;
        case -1: new_end += player_get_tape_length( self->play ); break;
        default: break;
    }
    player_loop_start( self->play, position );
    player_loop_end( self->play, new_end );
    printf("pos: %f %f\n",position,new_end);
}
void delay_ratio_cut( delay_t* self, int n, int d )
{
    if( d==0 ){ return; } // div-by-zero

    if( player_get_looping( self->play ) ){ // slice up the loop
        float lsize = player_get_loop_size( self->play );
        float cut = lsize * (float)n / (float)d;
        float start = player_get_loop_start( self->play );
        float dest_cut = cut + start;
        if( dest_cut > player_get_tape_length(self->play) ){ // wraps end of buffer
            float rev_cut = lsize + cut - 1.0;
            float end = player_get_loop_end( self->play );
            player_goto( self->play, end-rev_cut);
        } else { // plain loop
            player_goto( self->play, dest_cut );
        }
    } else { // slice up the whole buffer
        float tape_len = player_get_tape_length( self->play );
        player_goto( self->play, tape_len * (float)n / (float)d );
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
    return player_get_loop_size(self->play) / delay_get_rate( self );
}

float delay_get_length( delay_t* self )
{
    return player_get_loop_size(self->play) / player_get_tape_length( self->play);
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
    return player_get_goto( self->play );
}

float delay_get_lowpass( delay_t* self )
{
    return self->lpf;
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

static void mac_v( buffer_interface_t* self, float* io
                                    , int    origin
                                    , int    count
                                    , float  coeff )
{
    static float LAST_SAMP = 0.0;
    delay_t* d = (delay_t*)self->userdata;
    float* s = io;

    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    float* buffer = (float*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        // skipping bounds checks, instead relying on wrIPlayer's LEAD_IN protection
    // FIXME UNLOOP breaks without these. can optimize!
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        float bs = buffer[origin];

    // lp1. but this does weird things with the xfade :/ sounds ok though!?
    // sounds increasingly bad the shorter the time (greater % spent in xfade)
    // really need to get it inside of the wrIHead.
        LAST_SAMP  = LAST_SAMP + d->lpf * (bs - LAST_SAMP);
        LAST_SAMP *= coeff; // feedback level


        buffer[origin] = *s++ + LAST_SAMP;
        origin += dir;
    }
}
