#include "wrPhase.h"

#include <math.h>


// tests
#include <stdio.h>
void phase_show( phase_t p ){
    printf("%i %f\n",p.i,(double)p.f);
}
void phase_test( void ){
    printf("phase_null(): ");
    phase_show( phase_null() );

    printf("phase_zero(): ");
    phase_show( phase_zero() );

    printf("phase_new(3,0.51): ");
    phase_show( phase_new(3,0.51) );

    printf("phase_from_double(3.51): ");
    phase_show( phase_from_double(3.51) );

    printf("phase_from_double(-3.51): ");
    phase_show( phase_from_double(-3.51) );

    printf("phase_add(3.14,3.14): ");
    phase_show(
        phase_add( phase_from_double(3.14)
                 , phase_from_double(3.14)));

    printf("phase_add(3.14,-3.14): ");
    phase_show(
        phase_add( phase_from_double(3.14)
                 , phase_from_double(-3.14)));

    printf("phase_add(3.14,-3.15): ");
    phase_show(
        phase_add( phase_from_double(3.14)
                 , phase_from_double(-3.15)));

    printf("phase_sub(-3.14,3.14): ");
    phase_show(
        phase_sub( phase_from_double(-3.14)
                 , phase_from_double(3.14)));

    printf("phase_sub(-3.14,-3.14): ");
    phase_show(
        phase_sub( phase_from_double(-3.14)
                 , phase_from_double(-3.14)));

    printf("phase_sub(-3.14,-3.15): ");
    phase_show(
        phase_sub( phase_from_double(-3.14)
                 , phase_from_double(-3.15)));

    printf("phase_mul_d(1.1,1): ");
    phase_show(
        phase_mul_d( phase_from_double(1.1)
                 , 1.0));

    printf("phase_mul(2.45,2.45): ");
    phase_show(
        phase_mul( phase_from_double(2.45)
                 , phase_from_double(2.45)));

    // printf("phase_mul(1.1,1): ");
    // phase_show(
    //     phase_mul( phase_from_double(1.1)
    //              , 1.0));

}


////////////////////////////////////////
// phase_t helpers
phase_t phase_null( void ){
    return (phase_t){ .i = -1, .f = 0.0 };
}

phase_t phase_zero( void ){
    return (phase_t){ .i = 0.0, .f = 0.0 };
}

phase_t phase_new( int sample, float subsample ){
    return (phase_t){ .i = sample, .f = subsample };
}

phase_t phase_from_double( double d ){
    return (phase_t){ .i = (int)d
                    , .f = (float)(d - (double)(int)d)
                    };
}

double phase_to_double( phase_t p ){
    return (double)p.i + (double)p.f;
}

phase_t phase_add( phase_t a, phase_t b )
{
    phase_t p = { .i = a.i + b.i
                , .f = a.f + b.f
                };
    while(p.f >= 1.0){
        p.i++;
        p.f -= 1.0;
    }
    while(p.f < 0.0){
        p.i--;
        p.f += 1.0;
    }
    return p;
}

phase_t phase_sub( phase_t a, phase_t b )
{
    phase_t p = { .i = a.i - b.i
                , .f = a.f - b.f
                };
    while(p.f >= 1.0){
        p.i++;
        p.f -= 1.0;
    }
    while(p.f < 0.0){
        p.i--;
        p.f += 1.0;
    }
    return p;
}

// dot product
// caution: if either integer can't be represented by a double, it will fail
phase_t phase_mul( phase_t a, phase_t b )
{
    phase_t p = { .i = a.i * b.i
                , .f = a.f * b.f
                };
    // this will still truncate very large numbers
    double pi2 = ((double)a.i * (double)b.f);
    pi2 += (double)(a.f * (float)b.i);
    int pi3 = floor(pi2);
    p.i += pi3;
    p.f += (float)pi2 - (float)pi3;

// FIXME this could take a lot of repetitions with big numbers
    // better to split the integer from the fractional part and do all at once
    while(p.f >= 1.0){
        p.i++;
        p.f -= 1.0;
    }
    while(p.f < 0.0){
        p.i--;
        p.f += 1.0;
    }
    return p;
}

phase_t phase_mul_d( phase_t a, double b )
{
    return phase_mul( a, phase_from_double(b) );
}

bool phase_gt( phase_t a, phase_t b )
{
    bool retval;
    if( a.i > b.i ){ retval = true; }
    else if( a.i < b.i ){ retval = false; }
    else { // a.i == b.i
        if( a.f > b.f ){ retval = true; }
        else { retval = false; }
    }
    return retval;
}

bool phase_gte( phase_t a, phase_t b )
{
    bool retval;
    if( a.i > b.i ){ retval = true; }
    else if( a.i < b.i ){ retval = false; }
    else { // a.i == b.i
        if( a.f >= b.f ){ retval = true; }
        else { retval = false; }
    }
    return retval;
}

bool phase_lt( phase_t a, phase_t b )
{
    bool retval;
    if( a.i < b.i ){ retval = true; }
    else if( a.i > b.i ){ retval = false; }
    else { // a.i == b.i
        if( a.f < b.f ){ retval = true; }
        else { retval = false; }
    }
    return retval;
}

bool phase_lte( phase_t a, phase_t b )
{
    bool retval;
    if( a.i < b.i ){ retval = true; }
    else if( a.i > b.i ){ retval = false; }
    else { // a.i == b.i
        if( a.f <= b.f ){ retval = true; }
        else { retval = false; }
    }
    return retval;
}

bool phase_eq( phase_t a, phase_t b )
{
    bool retval;
    if( a.i == b.i
     && a.f == b.f ){ retval = true; }
    else { retval = false; }
    return retval;
}

bool phase_ez( phase_t a )
{
    bool retval;
    if( a.i == 0
     && a.f == 0.0 ){ retval = true; }
    else { retval = false; }
    return retval;
}
