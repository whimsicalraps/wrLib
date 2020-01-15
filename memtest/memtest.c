#include "wrBuffer.h"
#include "wrBufferInterface.h"
#include "wrClip.h"
#include "wrDelay.h"
#include "wrFilter.h"
#include "wrFuncGen.h"
#include "wrHistory.h"
#include "wrIHead.h"
#include "wrIPlayer.h"
#include "wrInterpolate.h"
#include "wrLpGate.h"
#include "wrOscSine.h"
#include "wrShaper.h"
#include "wrTransport.h"
#include "wrVtl.h"

#include <stdio.h>
#include <math.h>

void check_init( void )
{
    // wrBuffer
    buffer_deinit( buffer_init(0,0,buffer_interface_init()) );
    buffer_deinit( buffer_init(sizeof(float),0x1000,buffer_interface_init()) );
    // wrBufferInterface
    buffer_interface_deinit( buffer_interface_init() );
    // wrClip
    clip_soft_deinit( clip_soft_init_default() );
    // wrClip
    delay_deinit( delay_init(1024) );
    // wrFilter
    lp1_deinit( lp1_init() );
    lp1_a_deinit( lp1_a_init() );
    switch_ramp_deinit( switch_ramp_init() );
    awin_deinit( awin_init(3) );
    dc_deinit( dc_init() );
    svf_deinit( svf_init(0,48000) );
    // wrFuncGen
    function_deinit( function_init(1) );
    // wrFilter
    history_deinit( history_init(2,16) );
    // wrIHead
    ihead_deinit( ihead_init() );
    ihead_fade_deinit( ihead_fade_init() );
    // wrIPlayer
    player_deinit( player_init( NULL ) );
    buffer_t* b = buffer_init(0,0,buffer_interface_init());
    player_deinit( player_init( b ) );
    buffer_deinit( b );
    // wrLpGate
    lpgate_deinit( lpgate_init( 0, 0 ) );
    // wrOscSine
    sine_deinit( sine_init() );
    // wrTransport
    transport_deinit( transport_init() );
    // wrVtl
    vtl_deinit( vtl_init() );
}

int main( void )
{
    check_init();
    return 0;
}
