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

typedef enum{ QS_Priority_FIFO  // process requests as they are received
        // These two options maintain separate read & write buffers
            , QS_Priority_READ  // always process reads before writes
            , QS_Priority_WRITE // always process writes over reads
} QStream_Priority_t;


// TODO init to take a QStream_Priority_t for dynamic reordering of requests
// TODO init to take a flag for 'combine sequential accesses'
wrStream_t* QStream_init( int max_length, wrStream_t* stream );
void QStream_deinit( void );

void QStream_try( void );
