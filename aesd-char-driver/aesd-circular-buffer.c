/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer implementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 * 
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
 *entry_offset_byte_rtn = 0;
    size_t total_offset = 0;
    size_t entry_size=0;
    int entries_visited=0;
    struct aesd_buffer_entry *entry;
    if (buffer->full==false &&(buffer->in_offs==buffer->out_offs)) {
        return NULL;
    }
    // Iterate over all the valid entries in the buffer
    for (int i = buffer->out_offs; entries_visited < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; 
          entries_visited++,i=(i+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
        
        
        
        entry_size = buffer->entry[i].size;
        
        // Check if char_offset is within the current entry
        if (entry_size <= char_offset  ) {
            // If found, calculate the offset within the entry
            char_offset -=entry_size;
        }
        else
        {
        	entry_offset_byte_rtn=char_offset;
        	return &buffer->entry[i];
        }
	}
	*entry_offset_byte_rtn=(size_t)-1;
    // If we didn't find a matching entry, return NULL
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	const char *retval = NULL;


	if (buffer == NULL || add_entry == NULL) {
        	return retval;
    	}
    // Overwrite the oldest entry if the buffer is full
    if (buffer->full) {
    	 retval = buffer->entry[buffer->out_offs].buffptr;
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Add new entry at the current input position
    buffer->entry[buffer->in_offs] = *add_entry;

    // Advance the input position
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // Check if we've wrapped around and set the full flag if necessary
    if (buffer->in_offs == buffer->out_offs && buffer->full == false) {
        buffer->full = true;
    } 
    return retval;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
