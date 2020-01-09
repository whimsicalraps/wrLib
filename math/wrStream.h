#pragma once

#include <stdint.h>

// a wrapper over get/set functions into a stream - an 'interface' (?)
// uses a request/respond model to enable deferred access

// usage:
// the server should create a local wrStream_t
// server provides an 'init' function that returns wrStream_t*
// client accepts wrStream_t* and augments it with response & error fnptrs.

typedef enum{ DIR_READ
            , DIR_WRITE
} wrStream_DIR_t;

typedef int (*wrStream_Open_t)( const char* filepath, const char* mode );
typedef int (*wrStream_CB_t)( void );

// TODO should take a wrStream_PACKET_t literal-struct to avoid per-member copying
    // pass the struct on the stack, rather than using a reference
typedef int (*wrStream_RR_t)( wrStream_DIR_t direction
                            , int            location
                            , int            size_in_bytes
                            , uint8_t*       data );
typedef void (*wrStream_ER_t)( int errorcode, char* msg );
typedef struct{
    wrStream_Open_t open;
    wrStream_CB_t close;
    wrStream_CB_t busy;
    wrStream_RR_t request;
    wrStream_RR_t response;
    wrStream_ER_t error;
} wrStream_t;

typedef struct{
    wrStream_DIR_t direction;
    int            location;
    int            size_in_bytes;
    uint8_t*       data;
} wrStream_PACKET_t;
