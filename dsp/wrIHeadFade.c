#include "wrIHeadFade.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrIHead.h"

#include "../tables/delay_fades.h"


//////////////////////////////
// private declarations

static void ihead_fade_slew_heads( ihead_fade_t* self );
static void update_poke_slews( ihead_fade_t* self );
static void update_peek_phase( ihead_fade_t* self, float speed );
static void update_peek_phase2( ihead_fade_t* self, float speed );
static void update_peek_phase_head( ihead_t* self, float speed );


///////////////////////////////
// setup

ihead_fade_t* ihead_fade_init( void )
{
    ihead_fade_t* self = malloc( sizeof( ihead_fade_t ) );
    if( !self ){ printf("ihead_fade_init failed malloc\n"); return NULL; }

    self->fade_length = 0.01; // 10ms

    self->head[0] = ihead_init();
    self->head[1] = ihead_init();

    self->fade_active_head = 0;
    self->fade_phase = 0.0;
    self->fade_increment = 0.1;
    self->fade_countdown = 0;
    self->rec_slew = lp1_init();
    self->pre_slew = lp1_init();
    lp1_set_coeff( self->rec_slew, 0.01 ); // TODO configure slew time
    lp1_set_coeff( self->pre_slew, 0.01 ); // TODO configure slew time
    lp1_set_out( self->pre_slew, 1.0 );
    lp1_set_dest( self->pre_slew, 1.0 );
    self->fade_recording_dest = false;

    return self;
}
void ihead_fade_deinit( ihead_fade_t* self )
{
    ihead_deinit(self->head[0]);
    ihead_deinit(self->head[1]);
    free(self); self = NULL;
}


/////////////////////////////////////////
// params: setters

void ihead_fade_recording( ihead_fade_t* self, bool is_recording )
{
    //printf("fade_recording\n");
    if( is_recording != self->fade_recording_dest ){ // state change
        self->fade_recording_dest = is_recording;
        if( is_recording ){
            ihead_recording( self->head[0], true );
            ihead_recording( self->head[1], true );
        } // else: record state flips upon convergence of filter slews
        ihead_fade_slew_heads( self );
    }
}

void ihead_fade_rec_level( ihead_fade_t* self, float level )
{
    self->fade_rec_level = level;
    ihead_fade_slew_heads( self );
}

void ihead_fade_pre_level( ihead_fade_t* self, float level )
{
    self->fade_pre_level = level;
    ihead_fade_slew_heads( self );
}

void ihead_fade_jumpto( ihead_fade_t* self, buffer_t* buf, int phase, bool is_forward ){
    self->fade_active_head ^= 1; // flip active head
    ihead_jumpto( self->head[self->fade_active_head], buf, phase, is_forward );
    // initiate the xfade
    self->fade_phase = 0.0;
    float count = self->fade_length * 48000.0; // FIXME assume 48kHz samplerate
    self->fade_countdown = (int)count;
    if( count > 0 ){ self->fade_increment = 1.0 / count; }
}


//////////////////////////////////
// params: getters

bool ihead_fade_is_recording( ihead_fade_t* self ){
    return ihead_is_recording( self->head[0] );
}
float ihead_fade_get_rec_level( ihead_fade_t* self ){
    return self->fade_rec_level;
}
float ihead_fade_get_pre_level( ihead_fade_t* self ){
    return self->fade_pre_level;
}
int ihead_fade_get_location( ihead_fade_t* self ){
    return ihead_get_location( self->head[self->fade_active_head] );
}


///////////////////////////////////////
// signals

void ihead_fade_poke( ihead_fade_t*  self
                    , buffer_t*      buf
                    , float          speed
                    , float          input
                    )
{
    if( ihead_fade_is_recording( self ) ){ // skip poke if not recording
        ihead_t* hA = self->head[self->fade_active_head];
        float r = lp1_get_out( self->rec_slew );
        float p = lp1_get_out( self->pre_slew );
        if( self->fade_countdown > 0 ){
            ihead_t* hB = self->head[!self->fade_active_head];
            float mphase = 1.0 - self->fade_phase;

            // fade out
            ihead_rec_level( hB, r * rec_fade_LUT_get( mphase ) );
            float lut = pre_fade_LUT_get(self->fade_phase);
            float xf = 1.0 + lut*(p - 1.0);
            ihead_pre_level( hB, xf);
            ihead_poke( hB, buf, speed, input );

            // fade in
            ihead_rec_level( hA, r * rec_fade_LUT_get(self->fade_phase) );
            lut = pre_fade_LUT_get( mphase );
            xf = 1.0 + lut*(p - 1.0);
            ihead_pre_level( hA, xf);
            ihead_poke( hA, buf, speed, input );
        } else { // single head
            ihead_rec_level( hA, r );
            ihead_pre_level( hA, p );
            ihead_poke( hA, buf, speed, input );
        }
    }
}

int ihead_fade_update_phase( ihead_fade_t* self, float speed )
{
    update_poke_slews( self );
    update_peek_phase( self, speed );
    return ihead_fade_get_phase( self );
}

int ihead_fade_get_phase( ihead_fade_t* self )
{
    return self->head[ self->fade_active_head ]->rphase.i;
}

float ihead_fade_peek( ihead_fade_t* self, buffer_t* buf )
{
    float o;
    if( self->fade_countdown > 0 ){
        float out = ihead_peek( self->head[ !self->fade_active_head ], buf );
        float in  = ihead_peek( self->head[  self->fade_active_head ], buf );
        self->fade_phase += self->fade_increment; // move through xfade
        o  = out * equal_power_LUT_get(1.0 - self->fade_phase);
        o += in * equal_power_LUT_get(self->fade_phase);
    } else { // single head
        o = ihead_peek( self->head[ self->fade_active_head ], buf );
    }
    return o;
}

float* ihead_fade_peek_v( ihead_fade_t* self, float*    io
                                            , buffer_t* buf
                                            , float*    motion
                                            , int       size )
{
    if( self->fade_countdown >= size ){ // not end of xfade, assume countdown > 0
        ihead_t* h_out = self->head[!self->fade_active_head ];
        ihead_t* h_in  = self->head[ self->fade_active_head ];

        float fadeout[size];
        float fadein[size]; // TODO can save this buf by using the io array as tmp
        ihead_peek_v( h_out, fadeout, buf, motion, size );
        ihead_peek_v( h_in, fadein, buf, motion, size );

        for( int i=0; i<size; i++ ){
            self->fade_phase += self->fade_increment; // move through xfade
            io[i]  = fadeout[i] * equal_power_LUT_get(1.0 - self->fade_phase);
            io[i] += fadein[i] * equal_power_LUT_get(self->fade_phase);
        }
        self->fade_countdown -= size;
    } else if( self->fade_countdown > 0 ){ // half & half. split & recurse
        int fade_count = self->fade_countdown; // save a copy (it will change)
        ihead_fade_peek_v( self, io, buf, motion, self->fade_countdown );
        ihead_fade_peek_v( self, &io[fade_count], buf
                         , &motion[fade_count], size - fade_count );
    } else {
        ihead_t* h = self->head[ self->fade_active_head ];
        ihead_peek_v( h, io, buf, motion, size );
    }
    return io;
}

void ihead_fade_poke_v( ihead_fade_t*  self, buffer_t* buf
                                           , float*    motion
                                           , float*    io
                                           , int       size )
{
    for( int i=0; i<size; i++ ){
        ihead_fade_poke( self
                       , buf
                       , motion[i]
                       , io[i]
                       );
        update_poke_slews( self );
    }
}


////////////////////////////////////////
// helper fns

static void ihead_fade_slew_heads( ihead_fade_t* self )
{
    if( self->fade_recording_dest ){
        lp1_set_dest( self->rec_slew, self->fade_rec_level );
        lp1_set_dest( self->pre_slew, self->fade_pre_level );
    } else { // recording switched off
        lp1_set_dest( self->rec_slew, 0.0 );
        lp1_set_dest( self->pre_slew, 1.0 );
        // NOTE: ihead_recoding state will be flipped from step fn upon convergence
    }
}

static void update_poke_slews( ihead_fade_t* self )
{
    // update slews for rec/pre heads
    lp1_step_internal( self->rec_slew );
    lp1_step_internal( self->pre_slew );
    if( self->fade_recording_dest != ihead_fade_is_recording( self ) ){
        if( lp1_converged( self->rec_slew ) ){
            ihead_recording( self->head[0], false );
            ihead_recording( self->head[1], false );
        }
    }
}

static void update_peek_phase( ihead_fade_t* self, float speed )
{
    { // update phase (active head)
        phase_t* p = &(self->head[ self->fade_active_head ]->rphase);
        p->f += speed;
        while( p->f >= 1.0 ){ p->i++; p->f -= 1.0; }
        while( p->f <  0.0 ){ p->i--; p->f += 1.0; }
    }
    if( self->fade_countdown > 0 ){ // update phase (inactive head)
        self->fade_countdown--;
        phase_t* p = &(self->head[!self->fade_active_head ]->rphase);
        p->f += speed;
        while( p->f >= 1.0 ){ p->i++; p->f -= 1.0; }
        while( p->f <  0.0 ){ p->i--; p->f += 1.0; }
    }
}

static void update_peek_phase2( ihead_fade_t* self, float speed )
{
    ihead_update_peek_phase( self->head[ self->fade_active_head ], speed );
    if( self->fade_countdown > 0 ){ // update phase (inactive head)
        self->fade_countdown--;
        ihead_update_peek_phase( self->head[!self->fade_active_head ], speed );
    }
}
