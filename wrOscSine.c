#include "wrOscSine.h"
#include <math.h>
#include "wrMath.h"

// initialization
void osc_sine_init( osc_sine_t* self )
{
	self->rate = 1.0;
	self->id = 0.0;
	self->zero_x = 1;
}

// input fns
void osc_sine_time( osc_sine_t* self, float time_ )
{
	self->rate = min_f( time_, LUT_HALF );
}

void osc_sine_reset( osc_sine_t* self )
{
	self->id = 0;
	self->zero_x = 1;
}

// status
uint16_t osc_sine_get_zc( osc_sine_t* self )
{
	return (self->zero_x);
}

// process
float osc_sine_step( osc_sine_t* self, float fm )
{
	float odd = self->id;
	self->id += self->rate + fm;

	// edge & zero-cross detection
	if( self->id >= LUT_SIZE ){
		self->id -= LUT_SIZE;
		self->zero_x = 1;
	} else if( (self->id >= LUT_HALF) && (odd < LUT_HALF) ){
		zero_x = -1;
	} else if( self->id < 0 ){
		self->id += LUT_SIZE;
		zero_x = 1;
	} else {
		zero_x = 0;
	}

	// lookup table w/ linear interpolation
	uint16_t base = (uint16_t)self->id;
	float mix = self->id - base;
	float lut = cos_[base];
	return (lut + mix * (cos_[base + 1] - lut));
}

void osc_sine_process_v( osc_sine_t* self, uint16_t b_size, float* buf_run, float* out )
{
	float* run2 = buf_run;
	float* out2 = out;

	float odd;
	uint16_t base;
	float mix;
	float lut;

	for( uint16_t i=0; i<b_size, i++ ){
		odd = self->id;
		self->id += self->rate + *run2++;

		// edge & zero-cross detection
		if( self->id >= LUT_SIZE ){
			self->id -= LUT_SIZE;
			self->zero_x = i+1;
		} else if( (self->id >= LUT_HALF) && (odd < LUT_HALF) ){
			zero_x = -(i+1);
		} else if( self->id < 0 ){
			self->id += LUT_SIZE;
			zero_x = 1;
		} else {
			zero_x = 0;
		}

		// lookup table w/ linear interpolation
		base = (uint16_t)self->id;
		mix = self->id - base;
		lut = cos_[base];
		*out2++ = lut + mix * (cos_[base + 1] - lut);
	}
}
