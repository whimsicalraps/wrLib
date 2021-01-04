#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef struct buffer{
    int   len; // length of current data
    void* b;   // data buffer
    void* interface; // defined below
} buffer_t;

typedef enum{ Buf_Type_Float
            , Buf_Type_S16
} Buf_Type_t;

typedef struct buffer_interface{
    // link to parent
    buffer_t* buf;
    Buf_Type_t datatype; // type used by buffer_t

    // user-provided fnptrs for access
    float* (*peek_v)( struct buffer_interface*, float*, int, int);
    void (*poke_v)( struct buffer_interface*, float*, int, int);
    void (*mac_v)( struct buffer_interface*, float*, int, int, float);
    bool (*request)( struct buffer_interface*, int );
    void (*free)( struct buffer_interface* );

    // store a user struct here for implementing their interface
    void* userdata;
} buffer_interface_t;

// TODO change to void* instead of buffer_interface_t* pointing at userdata instead
// saves dereferencing later on
typedef float* (*buffer_peek_v_t)( buffer_interface_t*, float*, int, int );
typedef void (*buffer_poke_v_t)( buffer_interface_t*, float*, int, int );
typedef void (*buffer_mac_v_t)( buffer_interface_t*, float*, int, int, float );
typedef bool (*buffer_request_t)( buffer_interface_t*, int );

// setup
buffer_t* buffer_init( size_t bytes_per_value
                     , int count
                     , buffer_interface_t* interface );
void buffer_deinit( buffer_t* self );

buffer_t* buffer_new( buffer_t* self, size_t bytes_per_value, int length );
buffer_t* buffer_load_and_own( buffer_t* self, float* buffer, int length );

// these accessors wrap a user-provided implementation through buffer_interface_t
float* buffer_peek_v( buffer_t* self, float* dst, int origin, int count );
void buffer_poke_v( buffer_t* self, float* dst, int origin, int count );
void buffer_mac_v( buffer_t* self, float* dst, int origin, int count, float coeff );
bool buffer_request( buffer_t* self, int location );
