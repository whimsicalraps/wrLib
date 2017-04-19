#include "wrMath.h"

// this one seems to have problems? perhaps a priority issue? causing glitches once per block_size
float max_f(float a, float b) {
	float out;

	if(a>b) { out = a; } // a is greater, return a
	else { out = b; } // a is not greater, return b

	return out;
}

float min_f(float a, float b) {
	float out;

	if(a<b) { out = a; } // a is less, return a
	else { out = b; } // a is not less, return b

	return out;
}

float lim_f(float in, float min, float max) {
	float out;

	if(in<min) { out = min; } // lower limit
	else if(in>max) { out = max; } // upper limit
	else { out = in; } // echo in range

	return out;
}
float wrap_f(float in, float min, float max) {
	float diff = max - min;

	while(in<min) { in += diff; }
	while(in>=max) { in -= diff; }

	return in;
}
float interp_lin_f(float in1, float in2, float mix) {
	return (in1 + mix * (in2 - in1));
}
float interp_lin_f_2d(float in1_x, float in2_x,
						float in1_y, float in2_y,
						float mix_x, float mix_y){
	float tmp, tmp2;
	tmp = in1_x + mix_x*(in2_x - in1_x);
	tmp2 = in1_y + mix_x*(in2_y - in1_y);
	return (tmp + mix_y*(tmp2 - tmp));
}
int32_t lim_i32(int32_t in, int32_t min, int32_t max) {
	int32_t out;

	if(in<min) { out = min; } // lower limit
	else if(in>max) { out = max; } // upper limit
	else { out = in; } // echo in range

	return out;
}
int32_t wrap_i32(int32_t in, int32_t min, int32_t max) {
	int32_t diff = max - min;

	while(in<min) { in += diff; }
	while(in>max) { in -= diff; }

	return in;
}
int16_t min_u16(uint16_t a, uint16_t b) {
	uint16_t out;

	if(a<b) { out = a; } // a is less, return a
	else { out = b; } // a is not less, return b

	return out;
}
uint8_t min_u8(uint8_t a, uint8_t b) {
	uint8_t out;

	if(a<b) { out = a; } // a is less, return a
	else { out = b; } // a is not less, return b

	return out;
}
uint8_t max_u8(uint8_t a, uint8_t b) {
	uint8_t out;

	if(a>b) { out = a; } // a is less, return a
	else { out = b; } // a is not less, return b

	return out;
}


/////////////////////////////////////
// block processing math functions //
/////////////////////////////////////

void lim_v32_32(int32_t* in, int32_t min, int32_t max, int32_t* out, uint16_t size) {
	int32_t* in2=in;
	int32_t* out2=out;

	for(uint16_t i=0; i<size; i++) {
		if(*in2 < min) {
			*out2++ = min; // lower limit
			*in2++;
		}
		else if(*in2 > max) {
			*out2++ = max; // upper limit
			*in2++;
		}
		else {
			*out2++ = *in2++; // echo in range
		}
	}
}
void add_v32_v32_sat24(int32_t* a, int32_t* b, int32_t* out, uint16_t size){
	int32_t* a2=a;
	int32_t* b2=b;
	int32_t* out2=out;

	for(uint16_t i=0; i<size; i++) {
		*out2 = (*a2++) + (*b2++);

		if(*out2 < MIN24b) {
			*out2++ = MIN24b;
		}
		else if(*out2 > MAX24b) {
			*out2++ = MAX24b; // upper limit
		}
		else { *out2++; }
	}
}

void muladd_v32_f_v32_sat24(int32_t* in, float mul, int32_t* add, int32_t* out, uint16_t size) {
	int32_t* in2 = in;
	int32_t* add2 = add;
	int32_t* out2 = out;

	for(uint16_t i=0; i<size; i++) {
			// multiply v32 by float (in * thrulevel)
			// add above(vf) to (vi)output
		*out2 = (int32_t)((float)(*in2++) * mul) + (*add2++);
			// saturate to 24b
		if(*out2 < MIN24b) {
			*out2++ = MIN24b;
		}
		else if(*out2 > MAX24b) {
			*out2++ = MAX24b; // upper limit
		}
		else { *out2++; }
	}
}


void lim_vf_f(float* in, float min, float max, float* out, uint16_t size) {
	float* in2=in;
	float* out2=out;

	for(uint16_t i=0; i<size; i++) {
		if(*in2 < min) { *out2++ = min; } // lower limit
		else if(*in2 > max) { *out2++ = max; } // upper limit
		else { *out2++ = *in2; } // echo in range
		*in2++;
	}
}

// set an array to a single value
void set_v32_32(int32_t b, int32_t* out, uint16_t size) {
	int32_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = b;
	}	
}

	//////////////////////////
	////////////////////// ADD

// add two arrays of floats sequentially
void add_vf_vf(float* a, float* b, float* sum, uint16_t size) {
	float* a2=a;
	float* b2=b;
	float* sum2=sum; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*sum2++ = (*a2++) + (*b2++);
	}
}

// increment float array by scalar
void add_vf_f(float* a, float b, float* sum, uint16_t size) {
	float* a2=a;
	float* sum2=sum; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*sum2++ = (*a2++) + b;
	}	
}
void sub_vf_f(float* a, float b, float* diff, uint16_t size) {
	float* a2=a;
	float* diff2=diff; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*diff2++ = (*a2++) - b;
	}	
}
	//////////////////////////
	////////////////////// MUL

// vector multiplication
void mul_vf_vf(float* a, float* b, float* product, uint16_t size) {
	float* a2=a;
	float* b2=b;
	float* product2=product; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*product2++ = (*a2++) * (*b2++);
	}
}

// vector x constant multiply
void mul_vf_f(float* a, float b, float* product, uint16_t size) {
	float* a2=a;
	float* product2=product; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*product2++ = (*a2++) * b;
	}
}
void muladd_vf_f_f(float* vin, float mul, float add, float* product, uint16_t size) {
	float* vin2=vin;
	float* product2=product; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*product2++ = (*vin2++) * mul + add;
	}
}
void muladd_vf_f_vf(float* vin, float mul, float* vadd, float* product, uint16_t size) {
	float* vin2=vin;
	float* vadd2=vadd;
	float* product2=product; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*product2++ = (*vin2++) * mul + (*vadd2++);
	}
}
void mac_vf_f_vf(float* vmul, float mul, float* vadd, uint16_t size) {
	float* vmul2=vmul;
	float* vadd2=vadd; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		(*vadd2++) += (*vmul2++) * mul;
	}
}

	//////////////////////////
	////////////////////// DIV

// vector x constant divion
void div_vf_f(float* a, float b, float* divd, uint16_t size) {
	float* a2=a;
	float* divd2=divd; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*divd2++ = (*a2++) / b;
	}	
}

	//////////////////////////
	/////////////////// INTERP

void interp_lin_f_v(float* a, float* b, float* c, float* out, uint16_t size) {
	float* a2=a;
	float* b2=b;
	float* c2=c;
	float* out2=out;

	for(uint16_t i=0;i<size;i++) {
		*out2++ = *a2++ + *c2++ * (*b2++ - *a2);
	}
}
	//////////////////////////
	//////////////// SET VALUE

// copy whole vector
void set_v8_v8(uint8_t* b, uint8_t* out, uint16_t size) {
	uint8_t* b2=b;
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*b2++);
	}
}

// set vector to a constant
void set_v8_8(uint8_t b, uint8_t* out, uint16_t size) {
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = b;
	}	
}

	//////////////////////////
	////////////////////// ADD

// add two vectors
void add_v8_v8(uint8_t* a, uint8_t* b, uint8_t* sum, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* b2=b;
	uint8_t* sum2=sum; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*sum2++ = (*a2++) + (*b2++);
	}
}

// shift vector by scalar
void add_v8_8(uint8_t* a, uint8_t b, uint8_t* sum, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* sum2=sum; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*sum2++ = (*a2++) + b;
	}	
}

void add_v32_32(int32_t* a, int32_t b, int32_t* sum, uint16_t size) {
	int32_t* a2=a;
	int32_t* sum2=sum; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*sum2++ = (*a2++) + b;
	}	
}

/*
void lim_vf_f(float* in, float min, float max, float* out, uint16_t size) {
	float* in2=in;
	float* out2=out;

	for(uint16_t i=0; i<size; i++) {
		if(*in2 < min) { *out2++ = min; } // lower limit
		else if(*in2 > max) { *out2++ = max; } // upper limit
		else { *out2++ = *in2; } // echo in range
		*in2++;
	}
}
*/

	//////////////////////////
	////////////////// BITWISE


// bitwise OR a vector with a single mask
void OR_v8_8(uint8_t* a, uint8_t mask, uint8_t* out, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*a2++) | mask;
	}	
}

// bitwise AND a vector with a single mask
void AND_v8_8(uint8_t* a, uint8_t mask, uint8_t* out, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*a2++) & mask;
	}	
}

// left bitshift
void SHL_v8_8(uint8_t* a, uint8_t shift, uint8_t* out, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*a2++) << shift;
	}	
}

// right bitshift
void SHR_v8_8(uint8_t* a, uint8_t shift, uint8_t* out, uint16_t size) {
	uint8_t* a2=a;
	uint8_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*a2++) >> shift;
	}	
}

// right bitshift
// about 30% faster!
void SHR_v32_32(int32_t* a, uint16_t shift, int32_t* out, uint16_t size) {
	int32_t* a2=a;
	int32_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = (*a2++) >> shift;
	}	
}
void SHRadd_v32_32(int32_t* a, uint16_t shift, int32_t add, int32_t* out, uint16_t size) {
	int32_t* a2=a;
	int32_t* out2=out; // point to start of arrays

	for(uint16_t i=0; i<size; i++) {
		*out2++ = ((*a2++) >> shift) + add;
	}	
}