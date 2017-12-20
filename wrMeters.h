#pragma once

#include <stdint.h>

#include "wrFilter.h"

typedef struct VU_meter {
	filter_lp1_t smooth;  // just a wrapper for a low-pass
} VU_meter_t;

// variable speed
void VU_init( VU_meter_t* self);
void VU_time( VU_meter_t* self, float slew);
float VU_step( VU_meter_t* self, float in );