#pragma once

#include <stdint.h>
#include <stdlib.h>

#define EXTRACT_HIST_LENGTH 2 // >= 1 (no delay)

typedef enum etrig
    { tr_none       = 0
    , tr_p_negative = 1
    , tr_p_same     = 2
    , tr_p_positive = 3
    , tr_n_negative = 5 // as for 1-3 w/ b3 masked negative
    , tr_n_same     = 6
    , tr_n_positive = 7
    } etrig_t;

typedef struct event_extract {
   // parameters
   uint8_t history_length;
   int8_t  hyst_time;
   float   tr_abs_level;
   float   tr_rel_level;
   float   cv_thresh_same;
   float   cv_thresh_opp;

   // state
   int8_t  hyst;          // hysteresis
   float   in_history[EXTRACT_HIST_LENGTH]; // input history
   etrig_t tr_state;      // the output
} event_extract_t;

// variable speed
event_extract_t* Extract_init( void );
void Extract_deinit( event_extract_t* e );
etrig_t Extract_cv_trigger( event_extract_t* e, float in );