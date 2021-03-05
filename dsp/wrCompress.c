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
    lp1_a_set_coeff( self->lp1, 0.001, 0.0001 );

    return self;
}

// static int THROTTLE = 4800;
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
        sc -= 0.51;
        sc = (sc < 0.0) ? 0.0 : sc; // ensure no gain boost
        // ratio multiplier
        // TODO soft-knee non-linearity
        // sc = 0.45*sc; // just slightly below clipping
        sc = 2.5*sc;
        float PRE = sc;
        // sc = (sc > 0.55) ? 0.55 : sc; // make sure we don't apply inverse gain
        // dual-k 1pole lowpass
        sc = fminf(1.0, lp1_a_step( self->lp1, sc ));
        float SMOOTH = sc;
        // convert from gain-reduction to gain-level
        sc = 1.0 - (sc*0.72); // reduces limit to 0.5x gain
        sc = fmaxf(0.5,sc);
        // if( --THROTTLE <= 0){ THROTTLE = 4800;
            // printf("%f %f %f\n",(double)PRE, (double)SMOOTH, (double)sc);}
        // apply gain-reduction
        // *b++ *= sc*0.5; // TESTING. reduces level by 50% to avoid clip
        *b++ *= sc;
        // save servo feedback with threshold applied
        sc = sc + 0.0; // TODO threshold
    }
    self->sidechain = sc; // saves the last gain multiplier
    return io;
}
