#include "wrIHeadFade.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrIHead.h"

#include "../tables/delay_fades.h"


//////////////////////////////
// private declarations

static void ihead_fade_slew_heads( ihead_fade_t* self );
static void update_poke_slews( ihead_fade_t* self );


///////////////////////////////
// setup

ihead_fade_t* ihead_fade_init( Buf_Map_Type_t map_type )
{
    ihead_fade_t* self = malloc( sizeof( ihead_fade_t ) );
    if( !self ){ printf("ihead_fade_init failed malloc\n"); return NULL; }

    self->fade_length = 0.01; // 10ms

    self->head[0] = ihead_init( map_type );
    self->head[1] = ihead_init( map_type );

    self->fade_active_head = 0;
    self->fade_phase_r = 0.0;
    self->fade_phase_w = 0.0;
    self->fade_increment = 0.1;
    self->fade_countdown_r = 0;
    self->fade_countdown_w = 0;
    self->rec_slew = lp1_init();
    self->pre_slew = lp1_init();
    lp1_set_coeff( self->rec_slew, 0.01 ); // TODO configure slew time
    lp1_set_coeff( self->pre_slew, 0.01 ); // TODO configure slew time
    lp1_set_out( self->pre_slew, 1.0 );
    lp1_set_dest( self->pre_slew, 1.0 );
    self->fade_pre_filter = 0.97; // TODO apply a lowpass filter
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

void ihead_fade_xfade_ms( ihead_fade_t* self, float time )
{
    if( time >= 0.0 ){
        self->fade_length = time;
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

void ihead_fade_pre_filter( ihead_fade_t* self, float coeff )
{
    self->fade_pre_filter = coeff;
    ihead_pre_filter( self->head[0], coeff );
    ihead_pre_filter( self->head[1], coeff );
}

void ihead_fade_jumpto( ihead_fade_t* self, buffer_t* buf, phase_t phase, bool is_forward ){
    // copy active head's filter state into inactive head, then switch
    // ihead_pre_filter_set( self->head[!self->fade_active_head]
        // , ihead_get_pre_filter_state( self->head[self->fade_active_head] ) );
    if( self->fade_length >= (1.0/48000.0) ){ // at least 1 sample of fade time
        self->fade_active_head ^= 1; // flip active head
        ihead_jumpto( self->head[self->fade_active_head], buf, phase, is_forward );
        // initiate the xfade
        self->fade_phase_r = 0.0;
        self->fade_phase_w = 0.0;
        float count = self->fade_length * 48000.0; // FIXME assume 48kHz samplerate
        self->fade_countdown_r = (int)count;
        self->fade_countdown_w = (int)count;

    // FIXME rather than set to a value, copy the filter output from the active head into new-active head
            // nb: the below function isn't hooked up atm.
        // ihead_pre_filter_set( self->head[self->fade_active_head], 0.0 ); // reset the filter memory
        self->fade_increment = 1.0 / count;
    } else { // don't switch heads if no xfade
        ihead_jumpto( self->head[self->fade_active_head], buf, phase, is_forward );
    }
}

void ihead_fade_align( ihead_fade_t* self, bool is_forward ){
    ihead_align( self->head[0], is_forward );
    ihead_align( self->head[1], is_forward );
}


//////////////////////////////////
// params: getters

phase_t ihead_fade_get_location( ihead_fade_t* self ){
    return ihead_get_location( self->head[self->fade_active_head] );
}
bool ihead_fade_is_recording( ihead_fade_t* self ){
    return ihead_is_recording( self->head[0] ); // both are the same
}
float ihead_fade_get_rec_level( ihead_fade_t* self ){
    return self->fade_rec_level;
}
float ihead_fade_get_pre_level( ihead_fade_t* self ){
    return self->fade_pre_level;
}


///////////////////////////////////////
// signals

void ihead_fade_poke( ihead_fade_t*  self
                    , buffer_t*      buf
                    , float          speed
                    , float          input
                    )
{
    // TODO update with LUT for lowpass value
    if( ihead_fade_is_recording( self ) ){ // skip poke if not recording
        ihead_t* hA = self->head[self->fade_active_head];
        float r = lp1_get_out( self->rec_slew );
        float p = lp1_get_out( self->pre_slew );
    // filter
        // float c = self->fade_pre_filter;
        if( self->fade_countdown_w > 0 ){
            ihead_t* hB = self->head[!self->fade_active_head];
            self->fade_phase_w += self->fade_increment;
            float mphase = 1.0 - self->fade_phase_w;

            // fade out
            ihead_rec_level( hB, r * rec_fade_LUT_get( mphase ) );
        // deactivated pre_fade
            // float lut = pre_fade_LUT_get(self->fade_phase_w);
            // float xf = 1.0 + lut*(p - 1.0);
            // ihead_pre_level( hB, xf);
            ihead_pre_level( hB, p);
        // filter
            // xf = 1.0 + lut*(c - 1.0);
            // ihead_pre_filter( hB, xf );
            ihead_poke( hB, buf, speed, input );

            // fade in
            ihead_rec_level( hA, r * rec_fade_LUT_get(self->fade_phase_w) );
        // deactivated pre_fade
            // lut = pre_fade_LUT_get( mphase );
            // xf = 1.0 + lut*(p - 1.0);
            // ihead_pre_level( hA, xf);
            ihead_pre_level( hA, p);
        // filter
            // xf = 1.0 + lut*(c - 1.0);
            // ihead_pre_filter( hA, xf );
            ihead_poke( hA, buf, speed, input );
            self->fade_countdown_w--;
        } else { // single head
            ihead_rec_level( hA, r );
            ihead_pre_level( hA, p );
            // ihead_pre_filter( hA, c );
            ihead_poke( hA, buf, speed, input );
        }
    }
    update_poke_slews( self );
}

float ihead_fade_peek( ihead_fade_t* self, buffer_t* buf, float speed )
{
    float o;
    if( self->fade_countdown_r > 0 ){
        ihead_t* h_out = self->head[!self->fade_active_head ];
        ihead_t* h_in  = self->head[ self->fade_active_head ];

        float out = ihead_peek( h_out, buf, speed );
        float in  = ihead_peek( h_in, buf, speed );

        self->fade_phase_r += self->fade_increment; // move through xfade
        o  = out * equal_power_LUT_get(1.0 - self->fade_phase_r);
        o += in * equal_power_LUT_get(self->fade_phase_r);

        self->fade_countdown_r--;

    } else { // single head
        ihead_t* h = self->head[ self->fade_active_head ];
        o = ihead_peek( h, buf, speed );
    }
    return o;
}

float* ihead_fade_peek_v( ihead_fade_t* self, float*    io
                                            , buffer_t* buf
                                            , float*    motion
                                            , int       size )
{
    if( self->fade_countdown_r >= size ){ // not end of xfade, assume countdown > 0
        ihead_t* h_out = self->head[!self->fade_active_head ];
        ihead_t* h_in  = self->head[ self->fade_active_head ];

        float fadeout[size];
        float fadein[size]; // TODO can save this buf by using the io array as tmp
        ihead_peek_v( h_out, fadeout, buf, motion, size );
        ihead_peek_v( h_in, fadein, buf, motion, size );

        for( int i=0; i<size; i++ ){

// FIXME doing this here means it will be static block_size steps for poke?!?!?!?

            self->fade_phase_r += self->fade_increment; // move through xfade
            io[i]  = fadeout[i] * equal_power_LUT_get(1.0 - self->fade_phase_r);
            io[i] += fadein[i] * equal_power_LUT_get(self->fade_phase_r);
        }
        self->fade_countdown_r -= size;
    } else if( self->fade_countdown_r > 0 ){ // half & half. split & recurse
        int fade_count = self->fade_countdown_r; // save a copy (it will change)
        ihead_fade_peek_v( self, io, buf, motion, self->fade_countdown_r );
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
    if( ihead_fade_is_recording( self ) ){ // skip poke if not recording
        for( int i=0; i<size; i++ ){


// TODO vectorize this!
    // decide if lp1 slews are going to be per-sample or per-block

            ihead_t* hA = self->head[self->fade_active_head];
            float r = lp1_step_internal( self->rec_slew );
            float p = lp1_step_internal( self->pre_slew );
            if( self->fade_countdown_w > 0 ){
                ihead_t* hB = self->head[!self->fade_active_head];
                self->fade_phase_w += self->fade_increment;
                float mphase = 1.0 - self->fade_phase_w;
                // if( self->fade_length > 0.005 ){
                if( self->fade_length > 0.005 ){
                    // fade out
                    ihead_rec_level( hB, r * rec_fade_LUT_get( mphase ) );
                    float lut = pre_fade_LUT_get(self->fade_phase_w);
                    float xf = 1.0 + lut*(p - 1.0);
                    ihead_pre_level( hB, xf);
                    ihead_poke( hB, buf, motion[i], io[i] );
                    // fade in
                    ihead_rec_level( hA, r * rec_fade_LUT_get(self->fade_phase_w) );
                    lut = pre_fade_LUT_get( mphase );
                    xf = 1.0 + lut*(p - 1.0);
                    ihead_pre_level( hA, xf);
                    ihead_poke( hA, buf, motion[i], io[i] );
                } else { // short delay times have no rec-fade
                    // fade out
                    ihead_rec_level( hB, r * rec_fade_LUT_get( mphase ) );
                    ihead_pre_level( hB, p);
                    ihead_poke( hB, buf, motion[i], io[i] );
                    // fade in
                    ihead_rec_level( hA, r * rec_fade_LUT_get(self->fade_phase_w) );
                    ihead_pre_level( hA, p);
                    ihead_poke( hA, buf, motion[i], io[i] );
                }
                self->fade_countdown_w--;
            } else { // single head
                ihead_rec_level( hA, r );
                ihead_pre_level( hA, p );
                ihead_poke( hA, buf, motion[i], io[i] );
            }
        }

        if( !self->fade_recording_dest ){
            if( lp1_converged( self->rec_slew ) ){
                ihead_recording( self->head[0], false );
                ihead_recording( self->head[1], false );
            }
        }

    } else { // not recording
        for( int i=0; i<size; i++ ){ // TODO is this necessary?
            // TODO can optimize this to step by a 'block' of stages
            lp1_step_internal( self->rec_slew );
            lp1_step_internal( self->pre_slew );
        }
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
