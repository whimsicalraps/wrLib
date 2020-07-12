#include "wrDelay.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrBufferInterface.h"

static void apply_rate( delay_t* self );
static float get_loop_samples( delay_t* self );
void mac_v( buffer_interface_t* self, float* io
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
    self->play->transport->speeds.accel_standard = 0.001;
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

// set buffer length at current rate, to match seconds
// if time is not acheivable at current rate, mul/div rate by 2 to acheive it
void delay_time( delay_t* self, float samples )
{
    float rate = delay_get_rate(self);
    float adjusted_s = samples * rate;
    if( adjusted_s < 0.0 ){
        player_loop( self->play, false );
    } else if( adjusted_s == 0.0 ){
        printf("TODO set delay_time so that freq(0) == C3.\n");
    } else if( adjusted_s < 0.01 ){
        printf("TODO ignore because delay_time <10ms.\n");
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
        while( adjusted_s >= self->play->tape_end ){ // if too long, half sample rate
            rate *= 0.5;
            adjusted_s = samples * rate;
        }
        if( rate >= (1.0/16.0) ){
            delay_rate( self, rate );
            player_loop_start( self->play, 0 );
            player_loop_end( self->play, adjusted_s );
            player_loop( self->play, true );
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
        player_loop( self->play, false );
        return;
    } else { player_loop( self->play, true ); }
// nb: using doubles for better timing accuracy
    double bdiv = self->play->tape_end * fraction;
    int whole_divs = (int)((double)player_get_goto( self->play ) / bdiv);
    double start = (double)whole_divs * bdiv;
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

void delay_lowpass( delay_t* self, float coeff )
{
    self->lpf = (coeff < 0.0) ? 0.0 : (coeff > 1.0) ? coeff = 1.0 : coeff;
}

// getters
float delay_get_rate( delay_t* self )
{
    return self->rate;
}

float delay_get_time( delay_t* self )
{
    return get_loop_samples(self) / delay_get_rate( self );
}

float delay_get_length( delay_t* self )
{
    return get_loop_samples(self) / self->play->tape_end;
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

static float get_loop_samples( delay_t* self )
{
    float t=0.0;
    if( player_is_looping( self->play ) ){
        // FIXME handle outside loops (ie end before start)
        t = player_get_loop_end( self->play ) - player_get_loop_start( self->play );
    } else {
        t = self->play->tape_end;
    }
    return t;
}

void mac_v( buffer_interface_t* self, float* io
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
        float bs = buffer[origin];

    // lp1. but this does weird things with the xfade :/ sounds ok though!?
        LAST_SAMP  = LAST_SAMP + d->lpf * (bs - LAST_SAMP);
        LAST_SAMP *= coeff; // feedback level


        buffer[origin] = *s++ + LAST_SAMP;
        origin += dir;
    }
}
