#include "wrLpGate.h"

void lpgate_init( lpgate_t* gate, uint8_t hpf, uint8_t filter, uint16_t b_size )
{
	gate->hpf = hpf;
	gate->filter = filter;
	gate->b_size = b_size;

	gate->prev_lo = 0;
	gate->prev_hi = 0;
}

float lpgate_step( lpgate_t* gate, float level, float in )
{
	float out_lo, out_hi;

	if(gate->filter){ // BOTH MODE (LPF -> VOL)
		out_lo = gate->prev_lo +
					(level * 
					(in - gate->prev_lo));
		out_lo *= level/(0.1 + level) + LOG_VOL_CONST;
	} else {
		out_lo = gate->prev_lo +
					((0.5 + level*0.5) * 
					(in - gate->prev_lo));
		out_lo *= level;
	}

	if(gate->hpf){ // HPF ACTIVE
		out_hi = out_lo - gate->prev_lo + (HPF_COEFF * gate->prev_hi);
	} else{
		out_hi = out_lo;
	}
	gate->prev_lo = out_lo;
	gate->prev_hi = out_hi;
	return out_hi;
}

void lpgate_v( lpgate_t* gate, float* level, float* audio, float* out )
{
	float lpf[gate->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	// first sample
	if(gate->filter){ // BOTH MODE (LPF -> VOL)
		*lpf2 = gate->prev_lo +
					(*level2 * 
					(*in2++ - gate->prev_lo));
		*lpf2++ *= *level2/(0.1 + *level2) + LOG_VOL_CONST;
		level2++;
		for(i=1; i<(gate->b_size); i++){
			*lpf2 = *lpf3 +
						(*level2 * 
						(*in2++ - *lpf3));
			*lpf2++ *= *level2/(0.1 + *level2) + LOG_VOL_CONST;
			lpf3++;
			level2++;
		}
	} else { // VCA MODE (subtle LPF)
		*lpf2 = gate->prev_lo +
					((0.5 + *level2*0.5) * 
					(*in2++ - gate->prev_lo));
		*lpf2++ *= *level2++;
		for(i=1; i<(gate->b_size); i++){
			*lpf2 = *lpf3 +
						((0.5 + *level2*0.5) * 
						(*in2++ - *lpf3));
			*lpf2++ *= *level2++;
			lpf3++;
		}
	}

	lpf2 = lpf;
	if(gate->hpf){ // HPF ACTIVE
		lpf3 = lpf;
		float* out3 = out;
		*out2++ = *lpf2++ - gate->prev_lo + (HPF_COEFF * gate->prev_hi);
		for(i=1; i<(gate->b_size); i++){
			*out2++ = *lpf2++ - *lpf3++ + (HPF_COEFF * *out3++);
		}
		gate->prev_lo = *lpf3;
		gate->prev_hi = *out3;
	} else{ // PASS THROUGH
		for(i=0; i<(gate->b_size); i++){
			*out2++ = *lpf2++;
		}
		gate->prev_lo = *(--lpf2);
		gate->prev_hi = *(--out2);
	}
}