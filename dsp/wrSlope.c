#include "wrSlope.h"

#include <stdio.h>
#include <stdlib.h>

#include "wrBlocks.h"

///////////////////////////////////
// setup

slope_t* slope_init(void)
{
    slope_t* self = malloc( sizeof( slope_t ) );
    if( !self ){ printf("slope_t malloc failed\n"); return NULL; }

    self->now       = 0.0;
    self->dest      = 0.0;
    self->step      = 0.0;
    self->countdown = -1;

    return self;
}

void slope_deinit( slope_t* self )
{
    free(self); self = NULL;
}


/////////////////////////////
// setters

void slope_goto( slope_t* self, float dest, int duration )
{
    self->dest = dest;
    self->countdown = duration;
    float diff = self->dest - self->now;
    self->step = diff / self->countdown;
}


///////////////////////////////////////////
// getters

float slope_get_dest( slope_t* self ){ return self->dest; }
float slope_get_now( slope_t* self ){ return self->now; }


///////////////////////////////////
// signal

float slope_step( slope_t* self )
{
    if( self->countdown > 0 ){ // normal step
        self->countdown--;
        self->now += self->step;
    } else if( self->countdown == 0 ){ // convergence FIXME should be if, not elif?
        self->countdown--; // negative marks as converged
        self->now = self->dest;
    }
    return self->now;
}

float* slope_step_v( slope_t* self, float* buffer, int b_size )
{
    if( self->countdown >= 0 ){ // allow single step to catch convergence
        float* o = buffer;
        for( int i=0; i<b_size; i++ ){
            *o++ = slope_step( self );
        }
    } else { // already converged. just copy dest to buffer
        b_cp( buffer, self->now, b_size );
    }
    return buffer;
}
