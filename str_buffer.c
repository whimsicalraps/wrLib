#include "str_buffer.h"
#include <stdlib.h>
#include <string.h>

int8_t str_buffer_init(str_buffer_t* buf, uint16_t len)
{
	int8_t err = 0;
	buf->contents = malloc(sizeof(char) * len+1);
	if( buf->contents == NULL){ err = 1; }
	buf->contents[len] = 0; // null terminate n+1
	// empty when read == write ix
	buf->ix_read = 0;
	buf->ix_write = 0;
	buf->length = len; // count of chars
	buf->count = 0;

	return err;
}

void str_buffer_deinit( str_buffer_t* buf )
{
	free( buf->contents );
	buf->contents = NULL;
}

void str_buffer_enqueue(str_buffer_t* buf, char* s)
{
	while(*s) { // until we reach a NULL
		buf->contents[buf->ix_write++] = *s++;
		if(buf->ix_write >= buf->length) { buf->ix_write = 0; }
		buf->count++;
	}
}

char* str_buffer_dequeue(str_buffer_t* buf, uint16_t size)
{
	char* ret_str = &buf->contents[buf->ix_read];
	buf->count -= size;
	buf->ix_read += size;
	if(buf->ix_read >= buf->length) {buf->ix_read = 0;}
	return ret_str;
}

uint16_t str_buffer_len(str_buffer_t* buf)
{
	uint16_t ret_len = buf->count;
	if(ret_len + buf->ix_read > buf->length){
		ret_len = buf->length - buf->ix_read;
	}
	return ret_len;
}
