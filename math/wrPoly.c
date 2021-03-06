#include "wrPoly.h"

#include <stdlib.h>
#include <stdio.h> // printf



// FIXME
// list/queue logic is dumb
// refactor it with linked-lists for speed

// FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME 
// i already fucking fixed this shit in the JF3 branch.
// FML



// private declarations

void _free_voice( poly_alloc_t* self
				, uint8_t       index // into the busy_list
                , uint8_t       voice
                );
int8_t _assign_voice( poly_alloc_t* self
	                , int16_t       note
	                );
int8_t _steal_voice( poly_alloc_t* self
	               , int16_t       note
	               );

// public definitions
poly_alloc_t* poly_init( uint8_t voice_count )
{
    poly_alloc_t* self = malloc( sizeof( poly_alloc_t ) );
    if( !self ){ printf("poly malloc failed\n"); return NULL; }

	self->voice_count = voice_count;

	self->notes       = malloc(sizeof(int16_t) * voice_count);
	self->busy_count  = 0;
	self->busy_list   = malloc(sizeof(uint8_t) * voice_count);

	self->free_count  = voice_count;
	self->free_queue  = malloc(sizeof(uint8_t) * voice_count);
	for( uint8_t i=0; i<voice_count; i++ ){
		self->notes[i]      = 0x8000;
		self->busy_list[i]  = 255;
		self->free_queue[i] = i;
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

int8_t poly_assign_note( poly_alloc_t* self
                       , int16_t       note
                       )
{
	// note on
	if( note == (int16_t)0x8000 ){ return -1; } // invalid pitch

	// in future, search for note w same pitch

	// assign next free voice if there is one
	if( self->free_count ){
		return _assign_voice( self, note );
	} else {
		return _steal_voice( self, note );
	}
}

int8_t poly_kill_note( poly_alloc_t* self
                     , int16_t       note
                     )
{
	if( note == (int16_t)0x8000 ){ return -1; } // invalid pitch
	
	// search for match in self->notes
	for( uint8_t i=0; i<(self->busy_count); i++){
		uint8_t voice = self->busy_list[i];
		if(note == self->notes[voice]){ // note found in busy_list
			_free_voice( self, i, voice);
			return voice;
		}
	}
	return -1;
}

void _free_voice( poly_alloc_t* self
				, uint8_t       index // into the busy_list
                , uint8_t       voice
                )
{
	// forget pitch
	self->notes[voice] = (int16_t)0x8000; // invalid pitch

	// shuffle busy_list forward
	self->busy_count--;
	for( int i=index; i<(self->busy_count); i++ ){
		self->busy_list[i] = self->busy_list[i+1];
	}

	self->free_queue[self->free_count] = voice;
	self->free_count++;
}

int8_t _assign_voice( poly_alloc_t* self
	                , int16_t       note
	                )
{
	// take the front of the free queue (oldest)
	int8_t voice = self->free_queue[0];
	self->free_count--; // we took a voice
	// shift list forward
		// refactor to linked-list
	for( uint8_t i=0; i<(self->free_count); i++ ){
		self->free_queue[i] = self->free_queue[i+1];
	}

 	// add voice to end of busy_list
	self->busy_list[self->busy_count] = voice;
	self->busy_count++; // increase count

 	// save pitch
 	self->notes[voice] = note;

 	// return voice
 	return voice;
}

// steals front of busy_list and rotates it to the back of the list
int8_t _steal_voice( poly_alloc_t* self
	               , int16_t       note
	               )
{
	// steal front of busy list
	int8_t voice = self->busy_list[0];

	// shift list forward
		// refactor to linked-list
	for( uint8_t i=0; i<(self->busy_count-1); i++ ){
		self->busy_list[i] = self->busy_list[i+1];
	}
	self->busy_list[self->busy_count-1] = voice; // shifts new voice to end of list

	// save pitch
	self->notes[voice] = note;

	// return voice
	return voice;
}
