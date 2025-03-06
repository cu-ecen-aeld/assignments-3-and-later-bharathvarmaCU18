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
    /**
    * TODO: implement per description
    */
    
    //Empty condition
    if(buffer->out_offs == buffer->in_offs && buffer->full == false)
    {
        return NULL;
    }    
    
    int num_entries;
    int arr_index = buffer->out_offs;
    size_t total_size = 0;
    
    if(buffer->full)
    {
        num_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if(buffer->in_offs > buffer->out_offs)
    {
        num_entries = buffer->in_offs - buffer->out_offs;
    }
    else
    {
        //if my in_offs wrapped around
        num_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }
    
    for(int i = 0; i < num_entries; i++)
    {
        size_t entry_size = buffer->entry[arr_index].size;
        
        // If there are entries more than my char_offset, return the byte position and the entry structure
        if(total_size+entry_size > char_offset)
        {
            *entry_offset_byte_rtn = char_offset - total_size;
            return &buffer->entry[arr_index];
        }
        
        total_size += entry_size;
        arr_index = (arr_index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    
    
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    
    // Add the new entry in the in offset of the buffer
    buffer->entry[buffer->in_offs] = *add_entry;
    
    //buffer is full, increment the out_offs
    if(buffer->full)
    {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    
    //Buffer is full
    if(buffer->out_offs == buffer->in_offs)
    {
        buffer->full = true;
    }
    
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
