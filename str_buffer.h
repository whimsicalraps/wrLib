#pragma once

#include <stdio.h>
#include <stdint.h>

typedef struct str_buffer{
	char*    contents;
	int16_t  ix_read;
	int16_t  ix_write;
	uint16_t length;
	uint16_t count;
} str_buffer_t;

int8_t str_buffer_init(str_buffer_t* buf, uint16_t len);
void str_buffer_deinit( str_buffer_t* buf );
void str_buffer_enqueue(str_buffer_t* buf, char* s);
char* str_buffer_dequeue(str_buffer_t* buf, uint16_t size);
uint16_t str_buffer_len(str_buffer_t* buf);
uint8_t str_buffer_empty(str_buffer_t* buf);
