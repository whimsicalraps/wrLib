#include "wrBufferInterface.h"

#include <stdlib.h>
#include <stdio.h>


/////////////////////////////////////////
// defines

#define S16MAX  ((float)0x7FFF)
#define iS16MAX ((float)1.0/S16MAX)


///////////////////////////////////////////
// private declarations for export

// (float)buffer
static float* _peek_v( buffer_interface_t* self
              , float* io
              , int origin
              , int count
              );
static void _poke_v( buffer_interface_t* self
            , float* io
            , int origin
            , int count
            );
static void _mac_v( buffer_interface_t* self
           , float* io
           , int origin
           , int count
           , float coeff
           );
static void _map_v( buffer_interface_t* self
                  , float*     io
                  , int        origin
                  , int        count
                  , buf_map_t* map
                  );
// (int16_t)buffer
static float* _peek_v16( buffer_interface_t* self
              , float* io
              , int origin
              , int count
              );
static void _poke_v16( buffer_interface_t* self
            , float* io
            , int origin
            , int count
            );
static void _mac_v16( buffer_interface_t* self
           , float* io
           , int origin
           , int count
           , float coeff
           );
static void _map_v16( buffer_interface_t* self
                    , float*     io
                    , int        origin
                    , int        count
                    , buf_map_t* map
                    );
static bool _request( buffer_interface_t* self, int location );
static void _free( buffer_interface_t* self );


/////////////////////////////////////
// setup

buffer_interface_t* buffer_interface_init( Buf_Type_t type )
{
    buffer_interface_t* self = malloc( sizeof( buffer_interface_t ));
    if( !self ){ printf("couldn't malloc buffer_interface.\n"); return NULL; }

    self->datatype = type;

    self->buf     = NULL;
    if( self->datatype == Buf_Type_Float ){
        self->peek_v  = _peek_v;
        self->poke_v  = _poke_v;
        self->mac_v   = _mac_v;
        self->map_v   = _map_v;
    } else if( self->datatype == Buf_Type_S16 ){
        self->peek_v  = _peek_v16;
        self->poke_v  = _poke_v16;
        self->mac_v   = _mac_v16;
        self->map_v   = _map_v16;
    } else {
        printf("buffer_interface: unsupported datatype.\n");
    }
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

static float* _peek_v( buffer_interface_t* self
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

static void _poke_v( buffer_interface_t* self
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
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        buffer[origin] = *s++;
        origin += dir;
    }
}
#include <math.h>
static void _mac_v( buffer_interface_t* self
           , float* io
           , int origin
           , int count
           , float coeff
           )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    float* buffer = (float*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        float bs = buffer[origin];
        // buffer[origin] = *s++ + bs*coeff;
        buffer[origin] = fmaf(bs,coeff,*s++); // as above but uses hardware MAC
        origin += dir;
    }
}

static void _map_v( buffer_interface_t* self
                  , float*     io
                  , int        origin
                  , int        count
                  , buf_map_t* map )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    float* buffer = (float*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        // READ
        float dbuf = buffer[origin];
        // MAP
        dbuf = (*map->fn)(map->userdata, dbuf);
        // WRITE
        buffer[origin] = *s++ + dbuf;
        origin += dir;
    }
}

static float* _peek_v16( buffer_interface_t* self
              , float* io
              , int origin
              , int count
              )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    int16_t* buffer = (int16_t*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
    // FIXME using while here to allow big audio vectors & tiny delays
        // but this will have missed sample artifacts
        // technically the shortest delay allowed is (audio_vector - 4)
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }

        float dbuf = (float)buffer[origin] * iS16MAX; // TODO add dither?
        *s++ = dbuf;

        origin += dir;
    }
    return io;
}

static void _poke_v16( buffer_interface_t* self
            , float* io
            , int origin
            , int count
            )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    int16_t* buffer = (int16_t*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }

        int ts = *s++ * (float)S16MAX;
        ts = (ts > 0x7FFF) ? 0x7FFF
                : (ts < -0x8000) ? -0x8000 : ts;
        buffer[origin] = (int16_t)ts;

        origin += dir;
    }
}

static void _mac_v16( buffer_interface_t* self
           , float* io
           , int origin
           , int count
           , float coeff
           )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    int16_t* buffer = (int16_t*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        // READ
        float dbuf = (float)buffer[origin] * iS16MAX; // TODO add dither?
        // WRITE
        int ts = (*s++ + dbuf * coeff) * (float)S16MAX;
        ts = (ts > 0x7FFF) ? 0x7FFF
                : (ts < -0x8000) ? -0x8000 : ts;
        buffer[origin] = (int16_t)ts;
        origin += dir;
    }
}

static void _map_v16( buffer_interface_t* self, float* io
                                    , int    origin
                                    , int    count
                                    , buf_map_t* map )
{
    float* s = io;
    int dir = (count>=0) ? 1 : -1;
    int abscount = (count>=0) ? count : -count;
    int16_t* buffer = (int16_t*)self->buf->b;
    for( int i=0; i<abscount; i++ ){
        while( origin < 0 ){ origin += self->buf->len; }
        while( origin >= self->buf->len ){ origin -= self->buf->len; }
        // READ
        float dbuf = (float)buffer[origin] * iS16MAX; // TODO add dither?
        // MAP
        dbuf = (*map->fn)(map->userdata, dbuf);
        // WRITE
        int ts = (*s++ + dbuf) * (float)S16MAX;
        ts = (ts > 0x7FFF) ? 0x7FFF
                : (ts < -0x8000) ? -0x8000 : ts;
        buffer[origin] = (int16_t)ts;
        origin += dir;
    }
}

static bool _request( buffer_interface_t* self, int location )
{
    return true;
}

static void _free( buffer_interface_t* self )
{
    // DO NOTHING
    // implement this function to free your own allocated userdata
}
