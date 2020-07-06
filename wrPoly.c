#include "wrPoly.h"

#include <stdlib.h> // malloc, sizeof
#include <stdio.h> // printf


#define RESERVED_PITCH ((int16_t)0x8000)


///////////////////////////////////////
// helper declarations

static void release( poly_alloc_t* self , uint8_t voice );
static int8_t assign( poly_alloc_t* self , int16_t note );
static int8_t steal( poly_alloc_t* self , int16_t note );


///////////////////////////////////////
// initialization

poly_alloc_t* poly_init( uint8_t voice_count )
{
    poly_alloc_t* self = malloc( sizeof( poly_alloc_t ) );
    if( !self ){ printf("poly malloc failed\n"); return NULL; }

    self->voice_count = voice_count;

    self->notes       = malloc(sizeof(int16_t) * voice_count);
    if( !self->notes ){ printf("poly !self->notes\n"); return NULL; }
    self->busy_count  = 0;
    self->busy_list   = malloc(sizeof(uint8_t) * voice_count);
    if( !self->busy_list ){ printf("poly !self->busy_list\n"); return NULL; }

    self->free_count  = voice_count;
    self->free_first  = 0;
    self->free_queue  = malloc(sizeof(uint8_t) * voice_count);
    if( !self->free_queue ){ printf("poly !self->free_queue\n"); return NULL; }
    for( uint8_t i=0; i<voice_count; i++ ){
        self->notes[i]      = RESERVED_PITCH;
        self->busy_list[i]  = 255;
        self->free_queue[i] = 255;
    }
    return self;
}

void poly_deinit( poly_alloc_t* self )
{
    free(self->free_queue); self->free_queue = NULL;
    free(self->busy_list); self->busy_list = NULL;
    free(self->notes); self->notes = NULL;
    free(self); self = NULL;
}


///////////////////////////////////////
// public interface

int8_t poly_assign_note( poly_alloc_t* self
                       , int16_t       note
                       )
{
    if( note == RESERVED_PITCH ){ return -1; } // invalid pitch

    // TODO search for note w same pitch

    return (self->free_count)
            ? assign( self, note )
            : steal( self, note );
}

int8_t poly_kill_note( poly_alloc_t* self
                     , int16_t       note
                     )
{
    if( note == RESERVED_PITCH ){ return -1; } // invalid pitch

    // search for match in self->notes
    for( uint8_t i=0; i < self->voice_count; i++){
        if(note == self->notes[i]){ // found, so kill this
            release( self, i);
            return i;
        }
    }
    return -1;
}

///////////////////////////////////////
// helper fns

static void release( poly_alloc_t* self
                   , uint8_t       voice
                   )
{
    self->notes[voice] = RESERVED_PITCH; // forget pitch
    --self->busy_count; // decrement busy length

    uint8_t i = 0; // start search from oldest voice in the list
    while( voice != self->busy_list[i] ){ i++; } // find voice in aged-list
    // overwrite released voice & shift all newer voices forward in the list
    for(i; i < self->busy_count; i++){
        self->busy_list[i] = self->busy_list[i+1];
    }

    // enqueue note into free queue
    // add check for full-queue (shouldn't happen)
    uint8_t ix = (self->free_first + self->free_count++) % self->voice_count;
    self->free_queue[ix] = voice;
}

static int8_t assign( poly_alloc_t* self
                    , int16_t       note
                    )
{
    int8_t voice;

    // dequeue voice from free
    voice = self->free_first++;
    if(self->free_first >= self->voice_count){
        self->free_first = 0;
    }
    self->free_count--;

    self->busy_list[self->busy_count++] = voice; // add busy voice to end of list

    self->notes[voice] = note; // save pitch

    return voice;
}

static int8_t steal( poly_alloc_t* self
                   , int16_t       note
                   )
{
    int8_t voice = self->busy_list[0]; // steal front of busy list (oldest)

    // shift list forward
        // want to do a queue, but it varies in length?
    for( uint8_t i=0; i < (self->voice_count-1); i++ ){
        self->busy_list[i] = self->busy_list[i+1];
    }
    self->busy_list[self->voice_count-1] = voice;

    self->notes[voice] = note; // save pitch

    return voice;
}
