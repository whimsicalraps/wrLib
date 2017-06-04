#include "wrTrig.h"

// based on padé expansion
// accepts input values -3 to +3, outside this range needs to be clamped to +/-1
float tanh_fast(float in)
{
	float sq, out;

	if(in <= -3) { // clamp negative values
		out = -1;
	} else if(in >= 3) { // clamp positive values
		out = 1;
	} else {
		sq = in * in; // input squared
		out = in * (27 + sq) / (27 + 9 * sq);
	}

	return out;
}

void tanh_fast_v(float* in, float* out, uint16_t size)
{
	float* in2=in;
	float* out2=out;
	float sq;

	for(uint16_t i=0; i<size; i++) {
		if(*in2 <= -3) {
			*out2++ = -1.0;
		} else if(*in2 >= 3) {
			*out2++ = 1.0;
		} else {
			sq = (*in2) * (*in2); // input squared
			*out2++ = (*in2) * (27 + sq) / (27 + 9 * sq);
		}
		in2++;
	}
}