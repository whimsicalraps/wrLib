#include "wrCompress.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

compress_t* compress_init( void )
{
    compress_t* self = malloc(sizeof(compress_t));
    if(!self){ printf("compress malloc!\n"); return NULL; }

    self->sidechain = 1.0; // last gain multiplier
    self->lp1 = lp1_a_init();
    lp1_a_set_coeff( self->lp1, 0.0001, 0.00005 );

    return self;
}

float* compress_step_v(compress_t* self, float* io, int size)
{
    float* b = io;
    float sc = self->sidechain;
    for( int i=0; i<size; i++ ){
        // servo gain-reduction
        sc *= *b;
        // absolute val
        sc = fabsf(*b);
        // threshold
        sc -= 0.7;
        sc = (sc < 0.0) ? 0.0 : sc; // ensure no gain boost
        // ratio multiplier
        // TODO soft-knee non-linearity
        sc = 0.45*sc; // just slightly below clipping
        sc = (sc > 0.87) ? 0.87 : sc; // make sure we don't apply inverse gain
        // dual-k 1pole lowpass
        sc = lp1_a_step( self->lp1, sc );
        // convert from gain-reduction to gain-level
        sc = 1.0 - sc;
        // sc = sc*sc; // scales to apply closer to an audio-taper reduction
        // apply gain-reduction
        *b++ *= sc;
        // save servo feedback with threshold applied
        sc = sc + 0.0; // TODO threshold
    }
    self->sidechain = sc; // saves the last gain multiplier
    return io;
}
