#include "wrPoly.h"

#include <stdlib.h> // malloc, sizeof
#include <stdio.h> // printf


#define RESERVED_PITCH ((int16_t)0x8000)


///////////////////////////////////////
// helper declarations

static void release_voice( poly_alloc_t* self, int8_t voice );
static int8_t find_note( poly_alloc_t* self, int16_t note );
static int8_t find_item( poly_list_t* self, int8_t item );
static int8_t take_front( poly_list_t* self );
static void put_end( poly_list_t* self, int8_t item );
static void drop_index( poly_list_t* self, uint8_t ix );


///////////////////////////////////////
// initialization

poly_alloc_t* poly_init( uint8_t voice_count )
{
    poly_alloc_t* self = malloc( sizeof( poly_alloc_t ) );
    if( !self ){ printf("poly malloc failed\n"); return NULL; }

    self->voice_count = voice_count;

    self->notes = malloc(sizeof(int16_t) * voice_count);
    if( !self->notes ){ printf("poly !self->notes\n"); return NULL; }

    self->busy.count = 0;
    self->busy.list  = malloc(sizeof(int8_t) * voice_count);
    if( !self->busy.list ){ printf("poly !self->busy.list\n"); return NULL; }

    self->free.count = voice_count;
    self->free.list  = malloc(sizeof(int8_t) * voice_count);
    if( !self->free.list ){ printf("poly !self->free.list\n"); return NULL; }
    for( uint8_t i=0; i<voice_count; i++ ){
        self->notes[i]     = RESERVED_PITCH;
        self->busy.list[i] = -1;
        self->free.list[i] = i;
    }
    return self;
}

void poly_deinit( poly_alloc_t* self )
{
    free(self->free.list); self->free.list = NULL;
    free(self->busy.list); self->busy.list = NULL;
    free(self->notes); self->notes = NULL;
    free(self); self = NULL;
}


///////////////////////////////////////
// public interface

int8_t poly_assign_note( poly_alloc_t* self, int16_t note )
{
    if( note == RESERVED_PITCH ){ return -1; } // invalid pitch

    int8_t voice = take_front( (self->free.count)
                                    ? &self->free
                                    : &self->busy );
    put_end( &self->busy, voice );

    self->notes[voice] = note; // save pitch

    return voice;
}

void poly_assign_voice( poly_alloc_t* self, int8_t voice, int16_t note )
{
    if( voice < 0 || voice >= self->voice_count ){ return; } // invalid voice
    if( note == RESERVED_PITCH ){ return; } // invalid pitch

    release_voice( self, voice );
    put_end( &self->busy, voice );

    self->notes[voice] = note; // save pitch
}


int8_t poly_kill_note( poly_alloc_t* self, int16_t note )
{
    if( note == RESERVED_PITCH ){ return -1; } // invalid pitch

    int16_t voice = find_note( self, note );
    poly_kill_voice( self, voice );

    return voice;
}


void poly_kill_voice( poly_alloc_t* self, int8_t voice )
{
    if( voice < 0 || voice >= self->voice_count ){ return; }

    release_voice( self, voice );
    put_end( &self->free, voice );

    self->notes[voice] = RESERVED_PITCH; // forget pitch
}

///////////////////////////////////////
// helper fns

static void release_voice( poly_alloc_t* self, int8_t voice ){
    int8_t ix = find_item( &self->busy, voice );
    if( ix != -1 ){ // found in busy
        drop_index( &self->busy, ix );
    } else {
        ix = find_item( &self->free, voice ); // found in free
        drop_index( &self->free, ix );
    }
}

static int8_t find_note( poly_alloc_t* self, int16_t note ){
    for( uint8_t i=0; i < self->voice_count; i++ ){
        if( note == self->notes[i] ){ return i; }
    }
    return -1;
}

static int8_t find_item( poly_list_t* self, int8_t item ){
    for( uint8_t i=0; i < self->count; i++ ){
        if( item == self->list[i] ){ return i; }
    }
    return -1;
}

static int8_t take_front( poly_list_t* self )
{
    int8_t front = self->list[0];
    self->count--;

    // shift list forward
    for( uint8_t i=0; i < self->count; i++ ){
        self->list[i] = self->list[i+1];
    }
    return front;
}

static void put_end( poly_list_t* self, int8_t item )
{
    self->list[self->count] = item;
    self->count++;
}

static void drop_index( poly_list_t* self, uint8_t ix ){
    --self->count;
    for(; ix < self->count; ix++){
        self->list[ix] = self->list[ix+1];
    }
}
