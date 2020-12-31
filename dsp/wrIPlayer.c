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

static int tape_clamp( player_t* self, int location );
static bool player_is_going( player_t* self );
static void queue_goto( player_t* self, int sample );
static void order_loop_points( player_t* self );


///////////////////////////////
// setup

player_t* player_init( buffer_t* buffer )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self ){ printf("player malloc failed.\n"); return NULL; }

    self->head = ihead_fade_init();
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
    player_loop(self, true);
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
        self->tape_end = buffer->len;
        player_goto( self, 0 );
        player_loop_start( self, 0 );
        player_loop_end( self, self->tape_end );
    }
    return self;
}

void player_playing( player_t* self, bool is_play )
{
    transport_active( self->transport, is_play, transport_motor_standard );
}

void player_goto( player_t* self, int sample )
{
    if( self->buf ){
        if( buffer_request( self->buf, sample ) ){
            ihead_fade_jumpto( self->head
                             , self->buf
                             , sample
                             , (transport_get_speed_live( self->transport ) >= 0.0)
                             );
            self->queued_location = -1;
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
void player_head_order( player_t* self, bool play_before_erase ){
    self->play_before_erase = play_before_erase;
}
void player_loop( player_t* self, bool is_looping ){
    self->loop = is_looping;
}
void player_loop_start( player_t* self, int location ){
    self->loop_start = tape_clamp( self, location );
    order_loop_points(self);
}
void player_loop_end( player_t* self, int location ){
    self->loop_end = tape_clamp( self, location );
    order_loop_points(self);
}


///////////////////////////////////
// param getters

bool player_is_playing( player_t* self ){
    return transport_is_active( self->transport ); }
int player_get_goto( player_t* self ){
    return ihead_fade_get_location( self->head ); }
int player_get_queued_goto( player_t* self ){
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
bool player_is_looping( player_t* self ){ return self->loop; }
int player_get_loop_start( player_t* self ){ return self->loop_start; }
int player_get_loop_end( player_t* self ){ return self->loop_end; }


/////////////////////////////////////
// signals

static void request_queued_goto( player_t* self )
{
    int goto_dest = player_get_queued_goto( self );
    if( goto_dest != -1 ){ // check if a queued goto is ready
        //printf("try queued\n");
        player_goto( self, goto_dest );
    }
}

static void edge_checks( player_t* self )
{
    if( !player_is_going( self ) ){ // only edge check if there isn't a queued jump
        int new_phase = ihead_fade_get_location( self->head );
        int jumpto = -1;
        // TODO would it be better to jump exactly? rather than re: LEAD_IN
        // TODO self->loop could be a (-1,0,1) where -1 'jumps' over the loop (UNLOOP)
            // in this case, need to add extra case
// TEST THIS
        // Always test for tape edges before loop (incase of UNLOOP over the join)
        if( new_phase >= (self->tape_end - LEAD_IN) ){ jumpto = LEAD_IN; }
        else if( new_phase < LEAD_IN ){ jumpto = self->tape_end - LEAD_IN; }
        else if( self->loop ){ // apply loop brace
            if( new_phase >= self->o_loop_end ){ jumpto = self->o_loop_start; }
            else if( new_phase < self->o_loop_start ){ jumpto = self->o_loop_end; }
        }
    // UNLOOP
        /*
        else if( self->loop == UNLOOP ){
            // need to test against both the opposite edge, and the negative space
            // eg: (start > np >= end) -> start
            //     (end <= np < start) -> end
        }
        */
        if( jumpto >= 0 ){ // if there's a new jump, request it
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

static int tape_clamp( player_t* self, int location ){
    if( location < LEAD_IN ){ location = LEAD_IN; }
    if( location > (self->tape_end - LEAD_IN) ){ location = self->tape_end - LEAD_IN; }
    return location;
}

static void queue_goto( player_t* self, int sample ){
    if( sample != self->queued_location ){
        self->queued_location = sample;
    }
}

static bool player_is_going( player_t* self ){ return self->going; }

static void order_loop_points( player_t* self )
{
    // TODO handle UNLOOP version where we jump over it
    if( self->loop_start < self->loop_end ){ // forward
        self->o_loop_start = self->loop_start;
        self->o_loop_end   = self->loop_end;
    } else { // reverse
        self->o_loop_start = self->loop_end;
        self->o_loop_end   = self->loop_start;
    }
}
