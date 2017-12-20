#include "wrMeters.h"

void VU_init( VU_meter_t* self)
{
	lp1_init( &(self->smooth) );
	lp1_set_coeff( &(self->smooth), 0.018 );
}
void VU_time( VU_meter_t* self, float slew)
{
	lp1_set_coeff( &(self->smooth), slew );
}
float VU_step( VU_meter_t* self, float in )
{
	// simple sum of squares
	return lp1_step( &(self->smooth)
		           , in * in
		           );
}