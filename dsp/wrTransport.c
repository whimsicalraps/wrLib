#include "wrTransport.h"

#include <stdlib.h>
#include <stdio.h>
#include "../math/wrMath.h"
#include <math.h>


#define NUDGE_RATIO_PUSH (3.0/2.0)
#define NUDGE_RATIO_PULL (3.0/4.0)


///////////////////////////////
// private fns

static float get_speed_active( transport_t* self );


///////////////////////////////
// setup

transport_t* transport_init( void )
{
    transport_t* self = malloc( sizeof( transport_t ) );
    if( !self ){ printf("wrTransport malloc failed\n"); return NULL; }

    // default speed values
    transport_change_std_speeds( self
        , (std_speeds_t)
          { .max_speed      = 2.0
          , .accel_standard = 0.003
          , .accel_quick    = 0.05
          , .nudge_release  = 0.002
          }
    );

    self->speed_slew  = lp1_init();
    self->wind_slope = slope_init();
    self->nudge_slope = slope_init();
    transport_active( self, false, self->speeds.accel_standard );
    transport_speed( self, 1.0 );
    transport_offset( self, 0.0 );
    transport_nudge( self, Transport_Nudge_None );

    return self;
}

void transport_deinit( transport_t* self )
{
    lp1_deinit( self->speed_slew );
    free(self); self = NULL;
}


/////////////////////////////////
// setters

void transport_change_std_speeds( transport_t* self, std_speeds_t speeds )
{
    self->speeds = speeds;
}

void transport_active( transport_t*            self
                     , bool                    active
                     , transport_motor_speed_t slew
                     )
{
    self->active = active;
    lp1_set_coeff( self->speed_slew
                 , (slew == transport_motor_standard)
                        ? self->speeds.accel_standard
                        : self->speeds.accel_quick
                 );
    if( slew == transport_motor_instant ){
        lp1_set_out( self->speed_slew
                   , transport_get_speed( self )
                   );
    }
}

void transport_speed( transport_t* self, float speed )
{
    self->speed = lim_f( speed
                       , -self->speeds.max_speed
                       ,  self->speeds.max_speed
                       );
}

void transport_offset( transport_t* self, float offset )
{
    //TODO does this need to limited?
    self->offset = offset;
}

void transport_nudge( transport_t* self, Transport_Nudge_t n )
{
    switch( n ){
        case Transport_Nudge_Rewind:
            slope_goto( self->wind_slope, -2.0, 72000 ); break;
        case Transport_Nudge_Pull:
            slope_goto( self->nudge_slope, NUDGE_RATIO_PULL, 8000 ); break;
        case Transport_Nudge_None:
            slope_goto( self->wind_slope, 0.0, 6000 );
            slope_goto( self->nudge_slope, 1.0, 500 ); break;
        case Transport_Nudge_Push:
            slope_goto( self->nudge_slope, NUDGE_RATIO_PUSH, 8000 ); break;
        case Transport_Nudge_FastForward:
            slope_goto( self->wind_slope, 2.0, 72000 ); break;
    }
}


/////////////////////////////////
// getters

bool transport_is_active( transport_t* self ){ return self->active; }
float transport_get_speed( transport_t* self ){ return self->speed; }
float transport_get_offset( transport_t* self ){ return self->offset; }
float transport_get_speed_live( transport_t* self ){
    return lp1_get_out( self->speed_slew );
}


/////////////////////////////////
// signal

float transport_speed_step( transport_t* self )
{
    float speed;
    if( transport_is_active(self) ){ // musical nudge
        speed = fmaf( slope_step( self->nudge_slope )
                    , transport_get_speed(self)
                    , self->offset );
    } else { // transport is stopped, linear speed windup
        speed = self->offset + slope_step( self->wind_slope );
    }
    return lp1_step( self->speed_slew
                   , lim_f( speed
                          , -self->speeds.max_speed
                          , self->speeds.max_speed
                          )
                   );
}

float* transport_speed_v( transport_t* self
                        , float*       buffer
                        , int          b_size
                        )
{
    float* o = buffer;
    for( int i=0; i<b_size; i++ ){
        *o++ = transport_speed_step( self );
    }
    return buffer;
}


///////////////////////////////////
// helper fns

static float get_speed_active( transport_t* self ){
    return (transport_is_active( self ))
                ? transport_get_speed(self)
                : 0.0;
}
