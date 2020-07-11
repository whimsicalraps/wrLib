#pragma once

typedef struct{
    float now;
    float dest;
    float step;
    int countdown;
} slope_t;

// setup
slope_t* slope_init(void);
void slope_deinit( slope_t* self );

// setters
void slope_goto( slope_t* self, float dest, int duration ); // duration in samples

// getters
float slope_get_dest( slope_t* self );
float slope_get_now( slope_t* self );

// signal
float slope_step( slope_t* self );
float* slope_step_v( slope_t* self, float* buffer, int b_size );
