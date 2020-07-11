#include "wrIHead.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrInterpolate.h"
#include "../tables/delay_fades.h"

// necessary to keep read & write interpolation regions separate
#define REC_OFFSET (-2) // write head trails read head


//////////////////////////////
// private declarations
static float* push_input( ihead_t* self, float* buf, float in );
static int write( ihead_t* self, float rate, float input );
static void ihead_fade_slew_heads( ihead_fade_t* self );


/////////////////////////////////
// public interface

ihead_t* ihead_init( void )
{
    ihead_t* self = malloc( sizeof( ihead_t ) );
    if( !self ){ printf("ihead_init failed malloc\n"); return NULL; }

// WRITE / ERASE HEAD
    for( int i=0; i<4; i++ ){ self->in_buf[i] = 0.0; }
    self->in_buf_ix = 0;

    for( int i=0; i<OUT_BUF_LEN; i++ ){ self->out_buf[i] = 0.0; }
    self->out_phase = 0.0; // [0,1)

    self->write_ix = 0; // phase into the buffer_t

    ihead_recording( self, false );
    ihead_rec_level( self, 1.0 );
    ihead_pre_level( self, 0.0 );

// READ HEAD
    self->rphase.i = self->write_ix - REC_OFFSET;
    self->rphase.f = 0.0;

    return self;
}

void ihead_deinit( ihead_t* self )
{
    free(self); self = NULL;
}


///////////////////////////////
// add xfaded heads
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
void ihead_recording( ihead_t* self, bool is_recording ){
    self->recording = is_recording;
}
void ihead_rec_level( ihead_t* self, float level ){
    self->rec_level = level;
}
void ihead_pre_level( ihead_t* self, float level ){
    self->pre_level = level;
}
void ihead_jumpto( ihead_t* self, buffer_t* buf, int phase, bool is_forward ){
    self->write_ix = (is_forward) ? phase + REC_OFFSET : phase - REC_OFFSET;
    self->rphase.i = phase;
    self->rphase.f = 0.0;
}

// xfaded head
void ihead_fade_recording( ihead_fade_t* self, bool is_recording ){
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
void ihead_fade_rec_level( ihead_fade_t* self, float level ){
    self->fade_rec_level = level;
    ihead_fade_slew_heads( self );
}
void ihead_fade_pre_level( ihead_fade_t* self, float level ){
    self->fade_pre_level = level;
    ihead_fade_slew_heads( self );
}

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
bool ihead_is_recording( ihead_t* self ){ return self->recording; }
float ihead_get_rec_level( ihead_t* self ){ return self->rec_level; }
float ihead_get_pre_level( ihead_t* self ){ return self->pre_level; }
int ihead_get_location( ihead_t* self ){ return self->rphase.i; }

bool  ihead_fade_is_recording(  ihead_fade_t* self ){ return self->head[0]->recording; }
float ihead_fade_get_rec_level( ihead_fade_t* self ){ return self->fade_rec_level; }
float ihead_fade_get_pre_level( ihead_fade_t* self ){ return self->fade_pre_level; }
int   ihead_fade_get_location(  ihead_fade_t* self ){
    return self->head[self->fade_active_head]->rphase.i;
}

///////////////////////////////////////
// signals
void ihead_poke( ihead_t*  self
               , buffer_t* buf
               , float     speed
               , float     input
               )
{
    if( speed == 0.0 ){ return; }

    int nframes = write( self, speed, input * self->rec_level );
    if( nframes == 0 ){ return; } // nothing to do

    int nframes_dir = (speed >= 0.0) ? nframes : -nframes;
    if( self->recording ){
        float* y = self->out_buf; // y is the data to be written to the buffer_t
      // NOTE: using less efficient peek & poke to allow:
                // 1. easier to handle type conversion at a lower level
                // 2. easier to infer if a section of buffer needs to be rewritten
        float samps[nframes];
        buffer_peek_v( buf, samps, self->write_ix, nframes_dir );
        for( int i=0; i<nframes; i++ ){
// FIXME put the feedback filter here. just use lp1 for simplicity
            samps[i] *= self->pre_level; // erase head
            samps[i] += *y++;            // record head
        }
        buffer_poke_v( buf, samps, self->write_ix, nframes_dir );
    }
    self->write_ix += nframes_dir;
}

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
    // update slews for rec/pre heads
    lp1_step_internal( self->rec_slew );
    lp1_step_internal( self->pre_slew );
    if( self->fade_recording_dest != ihead_fade_is_recording( self ) ){
        if( lp1_converged( self->rec_slew ) ){
            ihead_recording( self->head[0], false );
            ihead_recording( self->head[1], false );
        }
    }

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

    return self->head[ self->fade_active_head ]->rphase.i;
}

// interpolate into the array of float*s
float ihead_peek( ihead_t* self, buffer_t* buf )
{
    float samps[4];
    buffer_peek_v( buf, samps, (self->rphase.i)-1, 4 );
    return interp_hermite_4pt( self->rphase.f, &samps[1] );
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


//////////////////////////////
// private funcs

static float* push_input( ihead_t* self, float* buf, float in )
{
    const int IN_BUF_MASK = 3; // ie IN_BUF_LEN - 1
    self->in_buf_ix = (self->in_buf_ix + 1) & IN_BUF_MASK;
    self->in_buf[self->in_buf_ix] = in;

    int i0 = (self->in_buf_ix + 1) & IN_BUF_MASK;
    int i1 = (self->in_buf_ix + 2) & IN_BUF_MASK;
    int i2 = (self->in_buf_ix + 3) & IN_BUF_MASK;
    int i3 =  self->in_buf_ix;

    buf[0] = self->in_buf[i0];
    buf[1] = self->in_buf[i1];
    buf[2] = self->in_buf[i2];
    buf[3] = self->in_buf[i3];

    return buf;
}

static int write( ihead_t* self, float rate, float input )
{
    float pushed_buf[4];
    push_input( self, pushed_buf, input );

    rate = (rate < 0.0) ? -rate : rate; // handle negative speeds

    float phi = 1.0/rate;
    float new_phase = self->out_phase + rate;
    int new_frames = (int)new_phase;
    // we want to track fractional output phase for interpolation
    // this is normalized to the distance between input frames
    // so: the distance to the first output frame boundary:
    if( new_frames ){
        // FIXME might need to be double? see: https://github.com/catfact/softcut/issues/6
        float f = 1.0 - self->out_phase;
        f *= phi; // normalized (divided by rate);

        int i=0;
        while(i < new_frames) {
            // distance between output frames in this normalized space is 1/rate
            self->out_buf[i] = interp_hermite_4pt( f, &(pushed_buf[1]) );
            f += phi;
            i++;
        }
    }
    // store the remainder of the updated, un-normalized output phase
    self->out_phase = new_phase - (float)new_frames;
    return new_frames; // count of complete frames written
}
