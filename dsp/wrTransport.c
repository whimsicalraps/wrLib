#include "wrTransport.h"

#include <stdlib.h>
#include <stdio.h> // printf
#include "wrMath.h"


transport_t* transport_init( void )
{
    transport_t* self = malloc( sizeof( transport_t ) );
    if( !self ){ printf("wrTransport malloc failed\n"); return NULL; }

    // default speed values
    transport_change_std_speeds( self
        , (std_speeds_t)
          { .max_speed      = 2.0
          , .accel_standard = 0.001
          , .accel_quick    = 0.05
          , .accel_seek     = 0.001
          , .accel_nudge    = 0.005
          , .nudge_release  = 0.002
          }
    );

    self->active = 0;

    self->speed_slew = lp1_init();
    lp1_set_coeff( self->speed_slew, self->speeds.accel_standard );
    self->speed_active   = 1.0;
    self->speed_inactive = 0.0;

    self->speed_manual = lp1_init();
    lp1_set_coeff( self->speed_manual, self->speeds.accel_seek );
    self->nudge       = 0.0;
    self->nudge_accum = 0.0;

    return self;
}


void transport_deinit( transport_t* self )
{
    lp1_deinit( self->speed_manual );
    lp1_deinit( self->speed_slew );
    free(self); self = NULL;
}


void transport_active( transport_t*            self
                     , bool                    active
                     , transport_motor_speed_t slew
                     )
{
    self->active = !!active;
    lp1_set_coeff( self->speed_manual
                 , (self->active)
                        ? self->speeds.accel_nudge
                        : self->speeds.accel_seek
                 );
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


void transport_speed_inactive( transport_t* self, float speed )
{
    float tmin = -self->speeds.max_speed;
    float tmax =  self->speeds.max_speed;
    self->speed_inactive = lim_f( speed
                                , tmin
                                , tmax
                                );
}


void transport_speed_active( transport_t* self, float speed )
{
    self->speed_active = lim_f( speed
                              , -self->speeds.max_speed
                              ,  self->speeds.max_speed
                              );
}


void transport_nudge( transport_t* self, float delta )
{
    self->nudge = delta;
}


bool transport_is_active( transport_t* self )
{
    return self->active;
}


void transport_change_std_speeds( transport_t* self, std_speeds_t speeds )
{
    self->speeds = speeds;
}


float transport_get_speed( transport_t* self )
{
    return (self->active)
                ? self->speed_active
                : self->speed_inactive;
}


float transport_speed_step( transport_t* self )
{
    lp1_set_dest( self->speed_slew
                , transport_get_speed( self )
                );

    // apply nudge / seek
    if( self->nudge ){
        if( self->active ){ // only nudge!
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -0.01
                                     ,  0.01
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -self->speeds.max_speed
                                     ,  self->speeds.max_speed
                                     );
        }
    } else {
        if( self->nudge_accum >= 0.0 ){
            self->nudge_accum = lim_f( self->nudge_accum
                                       - self->speeds.nudge_release
                                     , 0.0
                                     , self->speeds.max_speed
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum
                                       + self->speeds.nudge_release
                                     , -self->speeds.max_speed
                                     , 0.0
                                     );
        }
    }

    // limit nudged speed to +/-2.0
    float tmax =  self->speeds.max_speed;
    float tmin = -self->speeds.max_speed;
    lp1_set_out( self->speed_slew
          , lim_f( lp1_get_out( self->speed_slew ) + self->nudge_accum
                 , tmin
                 , tmax
                 ) );

    // slew toward new speed
    return lp1_step_internal( self->speed_slew );
}

float* transport_speed_block( transport_t* self
                            , float*       buffer
                            , int          b_size
                            )
{
    lp1_set_dest( self->speed_slew
                , transport_get_speed( self )
                );

    // apply nudge / seek
    if( self->nudge ){
        if( self->active ){ // only nudge!
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -0.01
                                     ,  0.01
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -self->speeds.max_speed
                                     ,  self->speeds.max_speed
                                     );
        }
    } else {
        if( self->nudge_accum >= 0.0 ){
            self->nudge_accum = lim_f( self->nudge_accum
                                       - self->speeds.nudge_release
                                     , 0.0
                                     , self->speeds.max_speed
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum
                                       + self->speeds.nudge_release
                                     , -self->speeds.max_speed
                                     , 0.0
                                     );
        }
    }

    // limit nudged speed to +/-2.0
    float tmax = self->speeds.max_speed;
    float tmin = -self->speeds.max_speed;
    lp1_set_out( self->speed_slew
          , lim_f( lp1_get_out( self->speed_slew ) + self->nudge_accum
                 , tmin
                 , tmax
                 ) );

    // slew toward new speed
    return lp1_step_c_v( self->speed_slew
                       , buffer
                       , b_size
                       );
}
