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

    // params
    player_playing( self->play, true );
    player_speed( self->play, 1.0 );
    player_recording( self->play, true );
    player_rec_level( self->play, 1.0 );
    player_pre_level( self->play, 0.9 );

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

void delay_feedback( delay_t* self, float feedback )
{
    player_pre_level( self->play, feedback );
}

// getters
float delay_get_rate( delay_t* self )
{
    return player_get_speed( self->play );
}

float delay_get_feedback( delay_t* self )
{
    return player_get_pre_level( self->play );
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
