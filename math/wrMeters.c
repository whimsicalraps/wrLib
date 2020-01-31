#include "wrMeters.h"

#include <stdlib.h> // malloc
#include <stdio.h> // printf

VU_meter_t* VU_init( void )
{
    VU_meter_t* self = malloc( sizeof( VU_meter_t ) );
    if( !self ){ printf("VU malloc failed\n"); return NULL; }

	self->smooth = lp1_init();
	lp1_set_coeff( self->smooth, 0.018 );

    return self;
}

void VU_deinit( VU_meter_t* self)
{
    lp1_deinit( self->smooth );
    free(self); self = NULL;
}

void VU_time( VU_meter_t* self, float slew)
{
	lp1_set_coeff( self->smooth, slew );
}

float VU_get( VU_meter_t* self )
{
    return lp1_get_out( self->smooth );
}

float VU_step( VU_meter_t* self, float in )
{
	// sum of squares
	return lp1_step( self->smooth
		           , in * in
		           );
}
