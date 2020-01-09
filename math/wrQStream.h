#pragma once

#include "wrStream.h"

/*
    CAVEAT EMPTOR

    This is *not* an object style library.
        Cannot be used on multiple simultaneous streams

    Due to how wrStream uses function pointers, I can't see how to have metadata
        (ie queue & packet data) associated with the stream, without having to call
        explicit 'qstream' functions.

    The goal is to have a QStream object spliced into wrStream_t without having
        to change any code in a project that already works with a synchronous stream

*/

wrStream_t* QStream_init( int max_length, wrStream_t* stream );
