/*
    A boilerplate buffer_interface_t

    Expects buffer_t to contain an array of floats to be accessed sequentially
*/

#pragma once

#include "wrBuffer.h"

buffer_interface_t* buffer_interface_init( void );
// Ownership of the buffer_interface passes to the buffer_t
// only use `deinit()` if you haven't passed ownership to a buffer
void buffer_interface_deinit( buffer_interface_t* self );
