#pragma once

#include <stdbool.h>
#include "wrFilter.h" // filter_lp1_t

typedef enum{ transport_motor_standard
            , transport_motor_quick
            , transport_motor_instant
} transport_motor_speed_t;

typedef struct{
    float max_speed;

    float accel_standard;
    float accel_quick;
    float accel_seek;
    float accel_nudge;

    float nudge_release;
} std_speeds_t;

typedef struct{
    std_speeds_t speeds;

    bool          active;

    filter_lp1_t* speed_slew; // smoothing for speed changes
    float         speed_active;
    float         speed_inactive;

    filter_lp1_t* speed_manual; // smoothing for manual changes
    float         nudge;      // how much are we currently nudging?
    float         nudge_accum;

} transport_t;


////////////////////////////////
// setup

transport_t* transport_init( void );
void transport_deinit( transport_t* self );


////////////////////////////////
// setters

void transport_active( transport_t*            self
                     , bool                    active
                     , transport_motor_speed_t slew
                     );
void transport_change_std_speeds( transport_t* self
                                , std_speeds_t speeds
                                );
void transport_speed_active( transport_t* self, float speed );
void transport_speed_inactive( transport_t* self, float speed );
void transport_nudge( transport_t* self, float delta );


/////////////////////////////////
// getters

bool transport_is_active( transport_t* self );
float transport_get_speed( transport_t* self );


/////////////////////////////////
// signal

float transport_speed_step( transport_t* self );
float* transport_speed_block( transport_t* self
                            , float* buffer
                            , int b_size
                            );
