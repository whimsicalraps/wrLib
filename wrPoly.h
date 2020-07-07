#pragma once

#include <stdint.h>

/* polyphonic note allocator
 * assumes all voices have same release time
 * up to 128 voices
 * pitch values are int16_t only
 * first attempts to assign to oldest, inactive voice
 * else steals longest held note
 * notes are assumed free upon beginning release stage
 *
 * once a note enters release it's pitch is forgotten
 * todo: note's maintain memory, for duplicate notes to
 *       trigger in same channel in case still sustaining.
 */

typedef struct{
    uint8_t  count;
    int8_t* list;
} poly_list_t;

typedef struct poly_alloc{
    uint8_t  voice_count;

    int16_t*    notes; // list of notes for the voices
    poly_list_t busy; // list of busy notes, (oldest..newest)
    poly_list_t free; // list of free notes, (oldest..newest)
    //uint8_t  busy_count;
    //uint8_t* busy_list;

    //uint8_t  free_count;
    //uint8_t* free_list;
} poly_alloc_t;

poly_alloc_t* poly_init( uint8_t voice_count );
void poly_deinit( poly_alloc_t* self );

int8_t poly_assign_note( poly_alloc_t* self , int16_t note );
void poly_assign_voice( poly_alloc_t* self, int8_t voice, int16_t note );
int8_t poly_kill_note( poly_alloc_t* self, int16_t note );
void poly_kill_voice( poly_alloc_t* self, int8_t voice );
