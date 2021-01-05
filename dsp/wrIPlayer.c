#include "wrIPlayer.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrBlocks.h"

/////////////////////////////////
// constants

#define LEAD_IN (64) // essentially an empty zone on either end of buffer
    // FIXME should be able to forgo LEAD_IN when looping whole buffer
    // in fact -- it shouldn't use 'goto' but just wrap the indices


///////////////////////////////
// private declarations

static phase_t tape_clamp( player_t* self, phase_t location );
static bool player_is_going( player_t* self );
static void queue_goto( player_t* self, phase_t sample );
static void order_loop_points( player_t* self );
static void calc_loop_size( player_t* self );


///////////////////////////////
// setup

player_t* player_init( buffer_t* buffer, Buf_Map_Type_t maptype )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self ){ printf("player malloc failed.\n"); return NULL; }

    self->head = ihead_fade_init(maptype);
    if( !self->head ){ printf("player head failed.\n"); return NULL; }

    self->transport = transport_init();
    if( !self->transport ){ printf("player transport failed.\n"); return NULL; }

    player_speed( self, 0.0 );
    player_speed_offset( self, 0.0 );
    player_load( self, buffer );
    player_playing( self, false );
    player_head_order( self, false );
    player_rec_level( self, 0.0 );
    player_pre_level( self, 1.0 );
    player_loop(self, 0 );
    self->going = false;
    return self;
}

void player_deinit( player_t* self )
{
    transport_deinit( self->transport );
    ihead_fade_deinit( self->head );
    free(self); self = NULL;
}


/////////////////////////////
// params

player_t* player_load( player_t* self, buffer_t* buffer )
{
    self->buf = buffer;
    if(self->buf){
        self->tape_end = phase_new( buffer->len, 0.0 );
        self->tape_end_lead = phase_new( buffer->len - LEAD_IN, 0.0 );
        self->tape_size = phase_new( buffer->len - 2*LEAD_IN, 0.0 );
        self->tape_start_lead = phase_new( LEAD_IN, 0.0 );
        player_goto( self, phase_zero() );
        player_loop_start( self, phase_zero() );
        player_loop_end( self, self->tape_end );
    }
    return self;
}

void player_playing( player_t* self, bool is_play )
{
    transport_active( self->transport, is_play, transport_motor_standard );
}

void player_goto_int( player_t* self, int sample )
{
    player_goto( self, (phase_t){ .i = sample, .f = 0.0 } );
}
void player_goto( player_t* self, phase_t sample )
{
    if( self->buf ){
        if( buffer_request( self->buf, sample.i ) ){
            ihead_fade_jumpto( self->head
                             , self->buf
                             , sample
                             , (transport_get_speed_live( self->transport ) >= 0.0)
                             );
            self->queued_location = phase_null();
            self->going = false;
        } else {
            self->going = true;
            queue_goto( self, sample );
        }
    } else {
        self->going = false;
    }
}

void player_speed( player_t* self, float speed ){
    transport_speed( self->transport, speed );
}
void player_speed_offset( player_t* self, float speed ){
    transport_offset( self->transport, speed );
}
void player_nudge( player_t* self, int amount ){
    transport_nudge( self->transport, amount );
}
void player_recording( player_t* self, bool is_record ){
    ihead_fade_recording( self->head, is_record );
    if( is_record ){ // realign write head when activating write
        ihead_fade_align( self->head
                        , (transport_get_speed_live( self->transport ) >= 0.0 )
                        );
    }
}
void player_rec_level( player_t* self, float rec_level ){
    ihead_fade_rec_level( self->head, rec_level );
}
void player_pre_level( player_t* self, float pre_level ){
    ihead_fade_pre_level( self->head, pre_level );
}
void player_pre_filter( player_t* self, float coeff ){
    ihead_fade_pre_filter( self->head, coeff );
}
void player_head_order( player_t* self, bool play_before_erase ){
    self->play_before_erase = play_before_erase;
}
// use ordering of loop points & direction of playback to determine loop style
// allows looping in both directions
// and loops that cross the 'join' of the tape loop.
void player_loop( player_t* self, int looping ){
    if( looping != 0 ){
        if( transport_get_speed_live( self->transport ) >= 0.0 ){ // forward
            if( phase_eq( self->loop_start, self->o_loop_start ) ){ // forward & ordered
                self->loop = 1;
            } else { // forward & out of order UNLOOP
                self->loop = 2;
            }
        } else { // reverse
            if( phase_eq( self->loop_start, self->o_loop_start ) ){ // reverse & ordered UNLOOP
                self->loop = 2;
            } else { // reverse & out of order LOOP
                self->loop = 1;
            }
        }
    } else {
        self->loop = 0;
    }
    calc_loop_size(self);
}
// FIXME temporary helpers for phase_t versions
void player_loop_start_int( player_t* self, int location ){
    player_loop_start( self, phase_new(location,0.0) );
}
void player_loop_end_int( player_t* self, int location ){
    player_loop_end( self, phase_new(location,0.0) );
}
void player_loop_start( player_t* self, phase_t location ){
    self->loop_start = tape_clamp( self, location );
    order_loop_points(self);
    calc_loop_size(self);
}
void player_loop_end( player_t* self, phase_t location ){
    self->loop_end = tape_clamp( self, location );
    order_loop_points(self);
    calc_loop_size(self);
}


///////////////////////////////////
// param getters

bool player_is_playing( player_t* self ){
    return transport_is_active( self->transport ); }
phase_t player_get_goto( player_t* self ){
    return ihead_fade_get_location( self->head ); }
phase_t player_get_queued_goto( player_t* self ){
    return self->queued_location; }
float player_get_speed( player_t* self ){
    return transport_get_speed( self->transport ); }
float player_get_speed_offset( player_t* self ){
    return transport_get_offset( self->transport ); }
float player_get_speed_live( player_t* self ){
    return transport_get_speed_live( self->transport ); }
bool player_is_recording( player_t* self ){
    return ihead_fade_is_recording( self->head ); }
float player_get_rec_level( player_t* self ){
    return ihead_fade_get_rec_level( self->head ); }
float player_get_pre_level( player_t* self ){
    return ihead_fade_get_pre_level( self->head ); }
bool player_is_head_order( player_t* self ){ return self->play_before_erase; }
int player_get_looping( player_t* self ){ return self->loop; }
int player_get_loop_start_int( player_t* self ){ return self->loop_start.i; }
int player_get_loop_end_int( player_t* self ){ return self->loop_end.i; }
phase_t player_get_loop_start( player_t* self ){ return self->loop_start; }
phase_t player_get_loop_end( player_t* self ){ return self->loop_end; }
int player_get_loop_size_int( player_t* self ){ return self->loop_size.i; }
phase_t player_get_loop_size( player_t* self ){ return self->loop_size; }
int player_get_tape_length_int( player_t* self ){
    return self->tape_size.i;
}
phase_t player_get_tape_length( player_t* self ){
    return self->tape_size;
}

/////////////////////////////////////
// queries

bool player_is_location_in_loop( player_t* self, phase_t location )
{
    bool inside = false;
    switch( player_get_looping(self) ){
        case 1: // LOOP
            if( phase_gte( location, self->o_loop_start )
             && phase_lt( location, self->o_loop_end ) ){ inside = true; }
            break;
        case 2: // UNLOOP
            if( phase_gte( location, self->o_loop_end )
             && phase_lt( location, self->o_loop_start) ){ inside = true; }
            break;
        default: break;
    }
    return inside;
}

// integer precision ok!
float player_position_in_loop( player_t* self, int location )
{
    int window = 0;
    float pos = 0.0;
    float size = (float)player_get_loop_size(self).i;
    switch( self->loop ){
        case 1: // LOOP
            window = location - self->o_loop_start.i;
            pos = (float)window / size;
            break;
        case 2: // UNLOOP
            if( location > self->o_loop_end.i ){ // first segment
                window = location - self->o_loop_end.i;
                pos = (float)window / size;
            } else { // second segment
                window = self->o_loop_start.i - location;
                pos = (float)window / size;
                pos = 1.0 - pos;
            }
            break;
        default: pos = -1.0; // means not looping
    }
    return pos;
}

int player_is_location_off_tape( player_t* self, phase_t location )
{
    int inside = false;
    if( phase_lt( location, self->tape_start_lead ) ){ inside = -1; }
    if( phase_gt( location, self->tape_end_lead) ){ inside = 1; }
    return inside;
}


/////////////////////////////////////
// signals

static void request_queued_goto( player_t* self )
{
    phase_t goto_dest = player_get_queued_goto( self );
    if( goto_dest.i != -1 ){ // check if a queued goto is ready
        player_goto( self, goto_dest );
    }
}

static void edge_checks( player_t* self )
{
    if( !player_is_going( self ) ){ // only edge check if there isn't a queued jump
        phase_t new_phase = ihead_fade_get_location( self->head );
        phase_t pend = self->tape_end_lead;
        phase_t pstart = self->tape_start_lead;
        phase_t jumpto = phase_null();
        // TODO would it be better to jump exactly? rather than re: LEAD_IN
        // Always test for tape edges before loop (incase of UNLOOP over the join)

        if( phase_gte( new_phase, pend ) ){ jumpto = pstart; }
        else if( phase_lt( new_phase, pstart ) ){ jumpto = pend; }
        else if( self->loop == 1 ){ // LOOP
            if( phase_gte( new_phase, self->o_loop_end ) ){ jumpto = self->o_loop_start; }
            else if( phase_lt( new_phase, self->o_loop_start ) ){ jumpto = self->o_loop_end; }
        } else if( self->loop == 2 ){ // UNLOOP
            if( phase_gt( new_phase, self->o_loop_start )
             && phase_lte( new_phase, self->o_loop_end ) ){ // in no man's land
                // choose destination by which is closer
                phase_t sdiff = phase_sub( new_phase, self->o_loop_start );
                phase_t ediff = phase_sub( self->o_loop_end, new_phase );
                if( phase_gt( sdiff, ediff ) ){ // close to end
                    jumpto = self->o_loop_start;
                } else { // close to start
                    jumpto = self->o_loop_end;
                }
            }
        }
        if( !phase_eq( jumpto, phase_null() ) ){ // if there's a new jump, request it
            player_goto( self, jumpto );
        }
    }
}


// this is an abstraction of a 'tape head'
float player_step( player_t* self, float in )
{
    if( !self->buf ){ return 0.0; } // no buffer available

    request_queued_goto( self );

    bool fwd = transport_get_speed_live( self->transport ) >= 0;
    float motion = transport_speed_step( self->transport );
    if( fwd != (motion >= 0) ){ // sign change in speed
        player_goto( self, player_get_goto(self) ); // reset head offset
    }

    float out = ihead_fade_peek( self->head, self->buf, motion );
    ihead_fade_poke( self->head
                   , self->buf
                   , motion
                   , in
                   );

    edge_checks( self );

    if( !self->play_before_erase && ihead_fade_is_recording( self->head ) ){
        out *= ihead_fade_get_pre_level( self->head );
    }
    return out;
}

float* player_step_v( player_t* self, float* io, int size )
{
    if( !self->buf ){ return b_cp( io, 0.0, size ); } // no buffer available

    request_queued_goto( self );

    { // motion[], outie[]
        bool last_dir = (transport_get_speed_live( self->transport ) >= 0);
        float motion[size];
        transport_speed_v( self->transport, motion, size );
        if( last_dir != (motion[size-1] >= 0) ){ // speed sign change this block
            player_goto( self, player_get_goto(self) ); // use the newest speed samp
            // FIXME will this cause it to jump by block_size samples?
        }

        float outie[size];
        ihead_fade_peek_v( self->head, outie, self->buf, motion, size );
        ihead_fade_poke_v( self->head
                         , self->buf
                         , motion
                         , io
                         , size );
        b_cp_v( io, outie, size );
    }

    edge_checks( self );

    if( !self->play_before_erase && ihead_fade_is_recording( self->head ) ){
        b_mul( io
             , ihead_fade_get_pre_level( self->head )
             , size );
    }
    return io;
}


//////////////////////////////////
// private funcs

static phase_t tape_clamp( player_t* self, phase_t location ){
    if( location.i < LEAD_IN ){ location = phase_new(LEAD_IN, 0.0); }
    phase_t end = self->tape_end_lead;
    if( phase_gt( location, end ) ){
        return end;
    }
    return location;
}

static void queue_goto( player_t* self, phase_t sample ){
    if( sample.i != self->queued_location.i ){
        self->queued_location = sample;
    }
}

static bool player_is_going( player_t* self ){ return self->going; }

static void order_loop_points( player_t* self )
{
    // TODO handle UNLOOP version where we jump over it
    if( phase_lt( self->loop_start, self->loop_end ) ){ // forward
        self->o_loop_start = self->loop_start;
        self->o_loop_end   = self->loop_end;
    } else { // reverse
        self->o_loop_start = self->loop_end;
        self->o_loop_end   = self->loop_start;
    }
}

static void calc_loop_size( player_t* self )
{
    switch( self->loop ){
        case 1: // LOOP
            self->loop_size = phase_sub( self->o_loop_end, self->o_loop_start );
            break;
        case 2: // UNLOOP
            self->loop_size = phase_sub( phase_add( self->o_loop_start // 2nd segment
                                                  , phase_sub( self->tape_end
                                                             , self->o_loop_end ) // 1st segment
                                                  )
                                       , phase_new( 2*LEAD_IN, 0.0 ) // less wrap protection
                                       );
            break;
        default: // OFF
            self->loop_size = player_get_tape_length( self );
            break;
    }
}
