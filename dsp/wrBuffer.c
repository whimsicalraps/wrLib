#include "wrBuffer.h"

#include <stdio.h>

////////////////////////////////
// private declarations

static void buffer_free( buffer_t* self );


////////////////////////////////
// public interface defns

buffer_t* buffer_init( size_t bytes_per_value
                     , int count
                     , buffer_interface_t* interface
                     )
{
    buffer_t* self = malloc( sizeof( buffer_t ) );
    if( !self ){ printf("buffer_init malloc failed.\n"); return NULL; }

    self->len = 0;
    self->b   = NULL;
    self->interface = interface;
    interface->buf = self; // back-link for interface to access buffer
    buffer_new( self, bytes_per_value, count );
    return self;
}

void buffer_deinit( buffer_t* self )
{
    buffer_free( self );
    buffer_interface_t* bi = ((buffer_interface_t*)(self->interface));
    if( bi->free != NULL ){ bi->free(bi); } // if the buffer_interface has a free fn
    free(bi); bi = NULL;
    free(self); self = NULL;
}

buffer_t* buffer_new( buffer_t* self, size_t bytes_per_value, int length )
{
    buffer_free( self );
    if( length ){ // zero-length just clears buffer
        self->b = calloc( length, bytes_per_value );
        if( !self->b ){
            printf("buffer_new calloc failed\n");
        } else {
            self->len = length;
        }
    }
    return self;
}

// WARNING! this function takes ownership of a buffer
// use it by passing a malloc'd pointer in directly
buffer_t* buffer_load_and_own( buffer_t* self, float* buffer, int length )
{
    buffer_free( self );
    if( buffer && length ){ // zero-length just clears buffer
        self->b   = buffer;
        self->len = length;
    }
    return self;
}

float* buffer_peek_v( buffer_t* self, float* dst, int origin, int count )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->peek_v)( i, dst, origin, count );
}

void buffer_poke_v( buffer_t* self, float* dst, int origin, int count )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->poke_v)( i, dst, origin, count );
}

void buffer_mac_v( buffer_t* self, float* dst, int origin, int count, float coeff )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->mac_v)( i, dst, origin, count, coeff );
}
void buffer_map_v( buffer_t* self, float* dst, int origin, int count, buf_map_t* map )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->map_v)( i, dst, origin, count, map );
}

bool buffer_request( buffer_t* self, int location )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->request)( i, location );
}

////////////////////////////////
// private defns

static void buffer_free( buffer_t* self )
{
    self->len = 0;
    if( self->b ){
        free(self->b);
        self->b = NULL;
    }
}
