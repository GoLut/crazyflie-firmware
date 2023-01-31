#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
// #include <assert.h>

#include "debug.h"


#include "circular_buffer.h"

////DISCLAIMER:
// This code is written by: https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
// Embedde artistry -- phillipjohnston @Github
// released under: Creative Commons Zero v1.0 Universal
 

// #pragma mark - Private Functions -

static inline uint8_t advance_headtail_value(uint8_t value, uint8_t max)
{
	return (value + 1) % max;
}

static void advance_head_pointer(cbuf_handle_t me)
{
	// assert(me);

	if(circular_buf_full(me))
	{
		me->tail = advance_headtail_value(me->tail, me->max);
	}

	me->head = advance_headtail_value(me->head, me->max);
	me->full = (me->head == me->tail);
}

// #pragma mark - APIs -

cbuf_handle_t circular_buf_init(uint8_t* buffer, uint8_t size, cbuf_handle_t cbuf)
{
	// assert(buffer && size);

	// cbuf_handle_t cbuf = malloc(sizeof(circular_buf_t));

	// assert(cbuf);

	cbuf->buffer = buffer;
	cbuf->max = size;
	circular_buf_reset(cbuf);

	// assert(circular_buf_empty(cbuf));

	return cbuf;
}

// void circular_buf_free(cbuf_handle_t me)
// {
// 	assert(me);
// 	free(me);
// }

void circular_buf_reset(cbuf_handle_t me)
{
	// assert(me);

	me->head = 0;
	me->tail = 0;
	me->full = false;
}

uint8_t circular_buf_size(cbuf_handle_t me)
{
	// assert(me);

	uint8_t size = me->max;

	if(!circular_buf_full(me))
	{
		if(me->head >= me->tail)
		{
			size = (me->head - me->tail);
		}
		else
		{
			size = (me->max + me->head - me->tail);
		}
	}

	return size;
}

uint8_t circular_buf_capacity(cbuf_handle_t me)
{
	// assert(me);

	return me->max;
}

void circular_buf_put(cbuf_handle_t me, uint8_t data)
{
	// assert(me && me->buffer);

	me->buffer[me->head] = data;

	advance_head_pointer(me);
}

int circular_buf_try_put(cbuf_handle_t me, uint8_t data)
{
	int r = -1;

	// assert(me && me->buffer);

	if(!circular_buf_full(me))
	{
		me->buffer[me->head] = data;
		advance_head_pointer(me);
		r = 0;
	}

	return r;
}

int circular_buf_get(cbuf_handle_t me, uint8_t* data)
{
	// assert(me && data && me->buffer);

	int r = -1;

	if(!circular_buf_empty(me))
	{
		*data = me->buffer[me->tail];
		me->tail = advance_headtail_value(me->tail, me->max);
		me->full = false;
		r = 0;
	}

	return r;
}

bool circular_buf_empty(cbuf_handle_t me)
{
	// assert(me);

	return (!circular_buf_full(me) && (me->head == me->tail));
}

bool circular_buf_full(cbuf_handle_t me)
{
	// assert(me);

	return me->full;
}

int circular_buf_peek(cbuf_handle_t me, uint8_t* data, uint8_t look_ahead_counter)
{
	int r = -1;
	uint8_t pos;

	// assert(me && data && me->buffer);

	// We can't look beyond the current buffer size
	if(circular_buf_empty(me) || look_ahead_counter > circular_buf_size(me))
	{
		DEBUG_PRINT("empty %d ----- ",circular_buf_empty(me));
		DEBUG_PRINT("LookAheadCounter: %d ---  ", look_ahead_counter);
		DEBUG_PRINT("larger than size %d \n", look_ahead_counter > circular_buf_size(me));
		return r;
	}

	pos = me->tail;
	for(uint8_t i = 0; i < look_ahead_counter; i++)
	{
		data[i] = me->buffer[pos];
		pos = advance_headtail_value(pos, me->max);
	}

	return 0;
}