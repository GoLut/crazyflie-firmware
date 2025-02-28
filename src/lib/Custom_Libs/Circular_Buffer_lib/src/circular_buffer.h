#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/////////
// based on code from embedded artistry 
/////////

/// Opaque circular buffer structure
// The definition of our circular buffer structure is visible to the user
typedef struct circular_buf_t
{
	uint16_t* buffer;
	size_t head;
	size_t tail;
	size_t max; // of the buffer
	bool full;
}circular_buf_t;

/// Handle type, the way users interact with the API
typedef circular_buf_t* cbuf_handle_t;

/// Pass in a storage buffer and size, returns a circular buffer handle
/// Requires: buffer is not NULL, size > 0 (size > 1 for the threadsafe
//  version, because it holds size - 1 elements)
/// Ensures: me has been created and is returned in an empty state
cbuf_handle_t circular_buf_init(uint16_t* buffer, size_t size, cbuf_handle_t cbuf);

/// Free a circular buffer structure
/// Requires: me is valid and created by circular_buf_init
/// Does not free data buffer; owner is responsible for that
// void circular_buf_free(cbuf_handle_t me);

/// Reset the circular buffer to empty, head == tail. Data not cleared
/// Requires: me is valid and created by circular_buf_init
void circular_buf_reset(cbuf_handle_t me);

/// Put that continues to add data if the buffer is full
/// Old data is overwritten
/// Note: if you are using the threadsafe version, this API cannot be used, because
/// it modifies the tail pointer in some cases. Use circular_buf_try_put instead.
/// Requires: me is valid and created by circular_buf_init
void circular_buf_put(cbuf_handle_t me, uint16_t data);

/// Put that rejects new data if the buffer is full
/// Note: if you are using the threadsafe version, *this* is the put you should use
/// Requires: me is valid and created by circular_buf_init
/// Returns 0 on success, -1 if buffer is full
int circular_buf_try_put(cbuf_handle_t me, uint16_t data);

/// Retrieve a value from the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns 0 on success, -1 if the buffer is empty
int circular_buf_get(cbuf_handle_t me, uint16_t* data);

/// CHecks if the buffer is empty
/// Requires: me is valid and created by circular_buf_init
/// Returns true if the buffer is empty
bool circular_buf_empty(cbuf_handle_t me);

/// Checks if the buffer is full
/// Requires: me is valid and created by circular_buf_init
/// Returns true if the buffer is full
bool circular_buf_full(cbuf_handle_t me);

/// Check the capacity of the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns the maximum capacity of the buffer
size_t circular_buf_capacity(cbuf_handle_t me);

/// Check the number of elements stored in the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns the current number of elements in the buffer
size_t circular_buf_size(cbuf_handle_t me);

/// Look ahead at values stored in the circular buffer without removing the data
/// Requires:
///		- me is valid and created by circular_buf_init
///		- look_ahead_counter is less than or equal to the value returned by circular_buf_size()
/// Returns 0 if successful, -1 if data is not available
int circular_buf_peek(cbuf_handle_t me, uint16_t* data, unsigned int look_ahead_counter);

// TODO: int circular_buf_get_range(circular_buf_t me, uint8_t *data, size_t len);
// TODO: int circular_buf_put_range(circular_buf_t me, uint8_t * data, size_t len);

#endif // CIRCULAR_BUFFER_H_