#pragma once

#include <stdbool.h>
#include "wrFilter.h" // filter_lp1_t
#include "wrSlope.h" // slope_t

typedef enum{ transport_motor_standard
            , transport_motor_quick
            , transport_motor_instant
} transport_motor_speed_t;

typedef struct{
    float max_speed;

    float accel_standard;
    float accel_quick;

    float nudge_release;
} std_speeds_t;

typedef enum{ Transport_Nudge_Rewind      = -2
            , Transport_Nudge_Pull        = -1
            , Transport_Nudge_None        = 0
            , Transport_Nudge_Push        = 1
            , Transport_Nudge_FastForward = 2
} Transport_Nudge_t;

typedef struct{
    std_speeds_t speeds;

    bool          active;

    filter_lp1_t* speed_slew; // smoothing for speed changes
    float         speed;
    float         offset; // operates regardless of active state

    slope_t*          nudge_slope;
    Transport_Nudge_t nudge;
} transport_t;


////////////////////////////////
// setup

transport_t* transport_init( void );
void transport_deinit( transport_t* self );


////////////////////////////////
// setters

void transport_change_std_speeds( transport_t* self
                                , std_speeds_t speeds
                                );
void transport_active( transport_t*            self
                     , bool                    active
                     , transport_motor_speed_t slew
                     );
void transport_speed( transport_t* self, float speed );
void transport_offset( transport_t* self, float offset );
void transport_nudge( transport_t* self, Transport_Nudge_t n );


/////////////////////////////////
// getters

bool transport_is_active( transport_t* self );
float transport_get_speed( transport_t* self );
float transport_get_offset( transport_t* self );
float transport_get_speed_live( transport_t* self );


/////////////////////////////////
// signal

float transport_speed_step( transport_t* self );
float* transport_speed_v( transport_t* self
                        , float* buffer
                        , int b_size
                        );
