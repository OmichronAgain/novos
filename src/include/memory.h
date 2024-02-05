#pragma once

#include <stdint.h>

template <typename T>
inline void memcpy(T* src, T* dest, uint32_t length)
{
    uint32_t offset = 0;
    while (offset < length)
    {
        dest[offset] = src[offset];
        offset++;
    }
}

template <typename T>
inline void memset(T value, T* dest, uint32_t length)
{
    uint32_t offset = 0;
    while (offset < length)
    {
        dest[offset] = value;
        offset++;
    }
}

/**
 * represents a block of heap memory which may be either free or occupied.
 * 
 * the size of a block can be calculated from its distance between it and
 * its `next` pointer. if `next` is `NULL` then the block should be considered
 * as being size zero.
 * 
 * `is_free` represents whether or not the block is empty and able to be allocated within
 * **/
#pragma pack (1)
struct nov_memory_frame
{
    nov_memory_frame* next = NULL;
    bool is_free = false;
};
#pragma pack (0)

/**
 * holds the start of the conceptual linked-list for memory management.
 * 
 * @attention ABSOLUTELY NEVER EVER EVER MODIFY THIS OUTSIDE THE MEMORY INITIALISER
 * 
 * **/
static nov_memory_frame* head_frame = NULL;

/**
 * configure the memory manager by initialising the head frame to cover the memory
 * area specified. do not call this more than once unless you want to totally
 * corrupt the memory map.
 * 
 * @param block_start pointer to the start of the available block
 * @param size size of the available block
 * **/
inline void init_memory_manager(void* block_start, uint32_t size)
{
    if (size < sizeof(nov_memory_frame) || block_start == NULL) return;

    // place a memory frame at the start of the provided block
    head_frame = (nov_memory_frame*)block_start;
    head_frame->is_free = true;
    // point it to an ending frame
    head_frame->next = (nov_memory_frame*)(((uint32_t)block_start+size)-sizeof(nov_memory_frame));

    // initialise the end frame to be a blank (size-zero) frame
    head_frame->next->next = NULL;
    head_frame->next->is_free = false;
}

/**
 * allocate some memory of a specified size. guaranteed to fail
 * if `init_memory_manager` hasn't been called
 * 
 * @param size size of memory to be allocated, in bytes
 * @return pointer to the newly allocated memory, if allocation was successful, or NULL if it failed
 * **/
inline void* malloc(uint32_t size)
{
    // keep track of the current and next blocks
    nov_memory_frame* current_block = head_frame;
    nov_memory_frame* next_block;
    uint32_t block_size;

    while (current_block != NULL)
    {
        // if this block isnt free, skip it
        if (!current_block->is_free) { current_block = current_block->next; continue; }
        next_block = current_block->next;
        // if the next block is NULL, then current_block is actually the blank end frame
        if (next_block == NULL) return NULL;

        // find the block size (i.e. the distance between the current frame and the next)
        block_size = (uint32_t)next_block-(uint32_t)current_block;
        if (block_size > size+sizeof(nov_memory_frame))
        {
            // we've found a free block big enough to hold the new memory allocation and the necessarry memory frame
            // make a new memory frame delineating the end of this newly allocated block
            nov_memory_frame* new_block = (nov_memory_frame*)((uint32_t)(current_block+1)+size);
            new_block->is_free = true;
            // insert into linked list
            new_block->next = next_block;
            current_block->next = new_block;
            // mark the current block as being occupied
            current_block->is_free = false;
            // return the address of the allocated block
            return (void*)(current_block+1);
        }
        // the block wasn't big enough, so lets move onto the next
        current_block = next_block;
    }

    return NULL;
}

// TODO: free
// TODO: consolidate
// TODO: view