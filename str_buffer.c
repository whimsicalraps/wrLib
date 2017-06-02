#include "str_buffer.h"
#include <stdlib.h>
#include <string.h>

void str_buffer_init(str_buffer_t* buf, uint16_t len)
{
	buf->contents = malloc(sizeof(uint8_t) * len+1);
	if( buf->contents == NULL){
		printf("str_buffer @9: !malloc()\n");
	}
	buf->contents[len] = 0; // null terminate n+1
	// empty when read == write ix
	buf->ix_read = 0;
	buf->ix_write = 0;
	buf->length = len; // count of chars
	buf->count = 0;
}
void str_buffer_enqueue(str_buffer_t* buf, uint8_t* s)
{
	while(*s) { // until we reach a NULL
		buf->contents[buf->ix_write++] = *s++;
		if(buf->ix_write >= buf->length) { buf->ix_write = 0; }
		buf->count++;
	}
}
uint8_t* str_buffer_dequeue(str_buffer_t* buf, uint16_t size)
{
	uint8_t* ret_str = &buf->contents[buf->ix_read];
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
