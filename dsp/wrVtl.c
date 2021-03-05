#include "wrVtl.h"

#include <stdlib.h>
#include <stdio.h> // printf

#include "wrMath.h" // lim_f_0_1

/////////////////////////////////////
// private declarations

static void vtl_update_params( vtl_t* self );


/////////////////////////////////////
// public interface

// setup

vtl_t* vtl_init( void ){
    vtl_t* self = malloc( sizeof( vtl_t ) );
    if(self == NULL){ printf("VTL: can't malloc!\n"); }

    self->dest  = 0.0;
    self->level = 0.0;
    self->vel   = 1.0;
    self->time     = 0.5;
    self->symmetry = 0.5;
    vtl_mode( self, vtl_mode_sustain );
    vtl_params( self, 100.0, 0.1 );
    return self;
}

void vtl_deinit( vtl_t* self ){
    free(self); self = NULL;
}

// params

void vtl_mode( vtl_t*     self
             , vtl_mode_t mode
             ){
    self->mode = mode;
    if(mode != vtl_mode_sustain){
        self->dest = 0.0;
    }
}

void vtl_time( vtl_t* self, float speed )
{
    self->time = lim_f_0_1(speed);
    vtl_update_params( self );
}

void vtl_symmetry( vtl_t* self, float ARratio )
{
    self->symmetry = lim_f_0_1(ARratio);
    vtl_update_params( self );
}

void vtl_params( vtl_t* self
               , float  speed
               , float  ARratio
               ){
    self->time = lim_f_0_1(speed);
    self->symmetry = lim_f_0_1(ARratio);
    vtl_update_params( self );
}


// unroll this in your loop if calling it per sample (otherwise it's minor improvement)
void vtl_dest( vtl_t* self
             , float dest
             ){
    if( dest < self->level
     && self->mode != vtl_mode_sustain ){ // just treat it as a new release as we have to go downward anyway
        self->dest = 0.0;
    } else {
        self->dest = dest; // set destination value
    }

    if( dest > nFloor ){
        self->vel = dest; // treat as 'velocity' input
    }
}


// getters

float vtl_get_level( vtl_t* self ){
    return self->level;
}


// signals

float vtl_step( vtl_t* self ){
	float slew_mod;
	float location = self->level;

	// difference between current & dest
	float sub_diff = self->dest - self->level;

	if(sub_diff > 0.0) { // rising
		slew_mod = self->rtime;
		slew_mod = min_f( slew_mod + slew_mod * location * location * 2.0
		                , 0.2); // some kind of hysteresis: out += 2 * in * previous^2
		self->level += slew_mod * sub_diff;


		if(sub_diff < nFloor) { // we're close enough! (~58dB)
			if(self->mode != vtl_mode_sustain) {
				self->dest = 0; // go toward zero if not sustaining
			}
		}
	} else { // falling
		slew_mod = self->ftime;
		slew_mod = min_f( slew_mod + slew_mod * location * location * 2.0
		                , 0.2); // some kind of hysteresis: out += 2 * in * previous^2
		self->level += slew_mod * sub_diff;

		if(sub_diff > -nFloor) { // if we've gone past the dest
			if(self->mode == vtl_mode_cycle) {
				self->dest = self->vel; // rise if cycling
			}
		}
	}
	return self->level; // needs to be limited to 0-1f
}

float* vtl_step_v( vtl_t* self
                 , float* out
                 , int    b_size
                 ){
	float slew_mod, slew_fix;
	float* out2=out;
	float* out3=out;
	uint16_t i;

	// difference between current & dest
	float sub_diff = self->dest - self->level;
                    // 1.0          // 0.007

	if( sub_diff > 0.0 ){ // rising
		if( sub_diff < nFloor ) { // call it even
			if( self->mode != vtl_mode_sustain ){
				self->dest = 0.0; // go toward zero
				slew_fix = self->ftime;
			} else { // sustain mode, so hold val
				// escape w/ fixed output value
				for( i=0; i<b_size; i++ ){
					*out2++ = self->dest;
				}
				self->level = self->dest; // save last val
				return out; // EARLY EXIT
			}
		} else { // normal rise
			slew_fix = self->rtime;
		}
	} else { // falling
		if( sub_diff > -nFloor ){ // call it even
			if( self->mode == vtl_mode_cycle ){
				if (self->dest == self->vel){ // AT MAX!
					self->dest = 0.0; // go to fall
					slew_fix = self->ftime;
				} else { // go to rise
					self->dest = self->vel;
					slew_fix = self->rtime;
				}
			} else { // hit dest
                if( self->dest == 0.0                 // dest was 'off' so we've reached the bottom
                 || self->mode == vtl_mode_sustain ){ // or sustaining, so filling at the sustain level
    				for( i=0; i<b_size; i++ ){
    					*out2++ = self->dest;
    				}
    				self->level = self->dest;
    				return out; // EARLY EXIT
                } else { // hit the peak from above, so now decay toward zero
                    self->dest = 0.0;
                    slew_fix = self->ftime;
                }
			}
		} else { // normal falling
			slew_fix = self->ftime;
		}
	}

	// some kind of hysteresis: out += 2 * in * previous^2
	slew_mod = slew_fix + slew_fix * self->level * self->level * 2.0;
	if(slew_mod > 0.2) { slew_mod = 0.2; } // limit rate to 1/5 per samp
	*out2++ = self->level + (slew_mod * sub_diff);

	for( i=1; i<b_size; i++ ){

		sub_diff = self->dest - *out3;

		if( sub_diff > 0.0 ){ // rising
			if( sub_diff < nFloor ){ // call it even
				if( self->mode != vtl_mode_sustain ){
					self->dest = 0.0; // go toward zero
					slew_fix = self->ftime;
				} else { // sustain mode, so hold val
					while( i++ < b_size ){
						*out2++ = self->dest;
					}
					self->level = self->dest; // save last val
					return out; // EARLY EXIT
				}
			} else { // normal rise
				slew_fix = self->rtime;
			}
		} else { // falling
			if(sub_diff > -nFloor) { // call it even
				if( self->mode == vtl_mode_cycle ){
					if (self->dest == self->vel){ // AT MAX!
						self->dest = 0.0; // go to fall
						slew_fix = self->ftime;
					} else { // go to rise
						self->dest = self->vel;
						slew_fix = self->rtime;
					}
                } else { // hit dest
                    if( self->dest == 0.0                 // dest was 'off' so we've reached the bottom
                     || self->mode == vtl_mode_sustain ){ // or sustaining, so filling at the sustain level
                        while( i++ < b_size ){
                            *out2++ = self->dest;
                        }
                        self->level = self->dest;
                        return out; // EARLY EXIT
                    } else { // hit the peak from above, so now decay toward zero
                        self->dest = 0.0;
                        slew_fix = self->ftime;
                    }
                }
			} else { // normal falling
				slew_fix = self->ftime;
			}
		}

		slew_mod = slew_fix + slew_fix * *out3 * *out3 * 2.0;
		if(slew_mod > 0.2) { slew_mod = 0.2; } // limit rate to 1/5 per samp
		*out2++ = (*out3++) + (slew_mod * sub_diff);
	}
	// save
	self->level = *out3;

	return out;
}


// private helpers

static void vtl_update_params( vtl_t* self )
{
    self->rtime = 0.5 / (0.998 * self->symmetry + 0.001);
    self->ftime = 1.0 / (2.0 - (1.0 / self->rtime));
    float ttime = lim_f( self->time * 0.007
                       , 0.000001
                       , 1.0
                       );
    self->rtime *= ttime;
    self->ftime *= ttime;
}
