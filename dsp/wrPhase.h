// sum type for representing large time values with subsample accuracy
// everything is passed on the stack, no allocation

#pragma once

#include <stdbool.h>

typedef struct{
    int   i;
    float f;
} phase_t;

////////////////////////////////////////
// phase_t helpers

void phase_test( void );

// generators
phase_t phase_null( void ); // returns a phase_t struct where i==-1
phase_t phase_zero( void ); // returns a phase_t struct where i==-1
phase_t phase_new( int sample, float subsample );
phase_t phase_from_double( double d );
double phase_to_double( phase_t p );

// arithmetic
phase_t phase_add( phase_t a, phase_t b ); // a + b
phase_t phase_sub( phase_t a, phase_t b ); // a - b
phase_t phase_mul( phase_t a, phase_t b ); // a * b
phase_t phase_mul_d( phase_t a, double b ); // a * b

// comparisons
bool phase_gt( phase_t a, phase_t b );     // a > b
bool phase_lt( phase_t a, phase_t b );     // a < b
bool phase_gte( phase_t a, phase_t b );    // a >= b
bool phase_lte( phase_t a, phase_t b );    // a <= b
bool phase_eq( phase_t a, phase_t b );     // a == b
bool phase_ez( phase_t a );                // a == 0
