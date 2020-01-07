#include "wrLpGate.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#define REPETITIONS 1000000
#define VECTORSIZE 32

float* baseline_lpgate_step( lpgate_t* self, float* levels, float* io, int size )
{
    for( int i=0; i<size; i++ ){
        io[i] = lpgate_step( self, levels[i], io[i] );
    }
    return io;
}

void main( void )
{
    clock_t _time;
    int size = VECTORSIZE;

    int hp=0; // highpass active? // FIXME set to 1 and it fails
    int p=0; // filter mode
    int F=0.1;
    int L=0.1;

    { // baseline
        lpgate_t* self = lpgate_init(hp,p);
        float buf[size]; for( int i=0; i<size; i++ ){ buf[i] = F; }
        float lvl[size]; for( int i=0; i<size; i++ ){ lvl[i] = L; }

        _time = clock();
        for( int i=0; i<REPETITIONS; i++ ){
            baseline_lpgate_step( self, lvl, buf, size );
        }
        _time = clock() - _time;
        printf("baseline: %f\n",((double)_time)/CLOCKS_PER_SEC);

        printf("baseline: %f\n",(double)buf[0]);
    }
    { // vectorized
        lpgate_t* self = lpgate_init(hp,p);
        float buf[size]; for( int i=0; i<size; i++ ){ buf[i] = F; }
        float lvl[size]; for( int i=0; i<size; i++ ){ lvl[i] = L; }

        _time = clock();
        for( int i=0; i<REPETITIONS; i++ ){
            lpgate_v( self, lvl, buf, size );
        }
        _time = clock() - _time;
        printf("vector: %f\n",((double)_time)/CLOCKS_PER_SEC);

        printf("vector: %f\n",(double)buf[0]);
    }
}

