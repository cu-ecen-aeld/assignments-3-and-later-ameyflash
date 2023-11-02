/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
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
    // get the current out_offs location
    uint8_t out_offs = buffer->out_offs;

    // while char_offset is greater than size of the current entry
    // reduce offset by size of current entry
    // advance the out_offs location
    while(char_offset >= buffer->entry[out_offs].size)
    {
        char_offset -= buffer->entry[out_offs].size;

        // advance the out_offs location
        if(out_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
        {
            out_offs = 0;
        }
        else
        {
            out_offs++;
        }
        
        // check if offset exceeds last byte
        if(out_offs == buffer->in_offs)
        {
            if(char_offset <= 0)
            {
                return NULL;
            }
        }

        // if entry doesn't exist
        // return NULL for no data found
        if(buffer->entry[out_offs].buffptr == NULL)
        {
            return NULL;
        }
    }
    
    // if entry doesn't exist
    // return NULL for no data found
    if(buffer->entry[out_offs].buffptr == NULL)
    {
        return NULL;
    }

    // store byte of the returned aesd_buffer_entry->buffptr member
    // corresponding to char_offset.
    *entry_offset_byte_rtn = char_offset;

    // return buffer entry
    return &(buffer->entry[out_offs]);
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char * aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char* ret_buffptr = NULL;
    
    // when buffer is not full
    if(buffer->full == false)
    {
        // store buffer entry
        buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
        buffer->entry[buffer->in_offs].size = add_entry->size;

        // check if in_offs location has reached end of buffer
        if(buffer->in_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
        {
            // go back to start of buffer
            buffer->in_offs = 0;
        }
        else
        {
            // increment in_offs location
            buffer->in_offs++;
        }

        // check and set if buffer is full
        if(buffer->in_offs == buffer->out_offs)
        {
            buffer->full = true;
        }
    }
    // when buffer is full
    else if(buffer->full == true)
    {
        ret_buffptr = buffer->entry[buffer->in_offs].buffptr;
        // store buffer entry
        buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
        buffer->entry[buffer->in_offs].size = add_entry->size;

        // increment in_offs and out_offs location
        buffer->in_offs++;
        buffer->out_offs++;
    }
    
    return ret_buffptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
