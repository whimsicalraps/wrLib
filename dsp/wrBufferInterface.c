#include "wrBufferInterface.h"

#include <stdlib.h>
#include <stdio.h>


///////////////////////////////////////////
// private declarations for export

float* _peek_v( buffer_interface_t* self
              , float* io
              , int origin
              , int count
              );
void _poke_v( buffer_interface_t* self
            , float* io
            , int origin
            , int count
            );
bool _request( buffer_interface_t* self, int location );
void _free( buffer_interface_t* self );


/////////////////////////////////////
// setup

buffer_interface_t* buffer_interface_init( void )
{
    buffer_interface_t* self = malloc( sizeof( buffer_interface_t ));
    if( !self ){ printf("couldn't malloc buffer_interface.\n"); return NULL; }

    self->buf     = NULL;
    self->peek_v  = _peek_v;
    self->poke_v  = _poke_v;
    self->request = _request;
    self->free    = _free;

    //self->userdata = NULL;

    return self;
}

void buffer_interface_deinit( buffer_interface_t* self )
{
    free( self ); self = NULL;
}


/////////////////////////////////////
// public api for export

float* _peek_v( buffer_interface_t* self
              , float* io
              , int origin
              , int count
              )
{

    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    float* buffer = (float*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
    // FIXME using while here to allow big audio vectors & tiny delays
        // but this will have missed sample artifacts
        // technically the shortest delay allowed is (audio_vector - 4)
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        *s++ = buffer[origin];
        origin += dir;
    }
    return io;
}

void _poke_v( buffer_interface_t* self
            , float* io
            , int origin
            , int count
            )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    float* buffer = (float*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        buffer[origin] = *s++;
        origin += dir;
    }
}

bool _request( buffer_interface_t* self, int location )
{
    return true;
}

void _free( buffer_interface_t* self )
{
    // DO NOTHING
    // implement this function to free your own allocated userdata
}
