#ifndef _LIST_H_
#define _LIST_H_

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "Heap.h"

typedef unsigned int uint_t;

/* user inputs for what list and fit to use */

char fit_type;
char list_type;

/* A _block BLOCK_T represents a chunk of memory in heap */
typedef struct _block
{
    size_t size;
    bool free;
    uint_t payload_index;
    struct _block *next;
    struct _block *prev;
} BLOCK_T;

/* An _elist ELIST_T for storing free blocks in LIFO fashion */
typedef struct _elist 
{
    BLOCK_T *block;
    struct _elist *next;
    struct _elist *prev;
} ELIST_T;


BLOCK_T *head;      // points to the head of the implicit list
ELIST_T *exp_list;  // points to the head of the explicitly created list

/*  */
void split(BLOCK_T *block, size_t size, ELIST_T *free_block)
{
    uint_t left_footer = block->payload_index + (size / sizeof(WORD));

    if(size % sizeof(WORD) > 0) left_footer++;
    if(size < sizeof(WORD)) left_footer++;

    // find the next header
    uint_t next_header = left_footer + 1;
    if(next_header % 2 == 0) next_header++;

    // find next footer
    uint_t next_footer = block->payload_index + block->size / sizeof(WORD);
    if(block->size % sizeof(WORD) > 0) next_footer++;

    // maintain footer bounds
    if(next_footer > heap_size / sizeof(WORD))
        next_footer = heap_size / sizeof(WORD);
    
    // find the next size
    uint_t next_size = (next_footer - next_header) * sizeof(WORD);

    if(next_size < sizeof(WORD) * 2)
        return;
    
    /* otherwise create a new free block */

    BLOCK_T *next_block = (BLOCK_T *)malloc(sizeof(BLOCK_T));
    next_block->size = next_size;
    next_block->free = true;
    next_block->payload_index = next_header + 1;

    block->free = false;
    block->size = size;

    if(block->next != NULL)
    {
        next_block->prev = block;
        next_block->next = block->next;
        block->next->prev = next_block;
        block->next = next_block;
    }
    else
    {
        next_block->prev = block;
        next_block->next = 0;
        block->next = next_block;
    }
    
    if(list_type == 'E') free_block->block = next_block;

    int cap = next_block->size / sizeof(WORD);
    if(next_block->size % sizeof(WORD) > 0) cap++;

    /* update the heap header and footer */

    int serial = (cap - 1) * 4;
    heap[next_block->payload_index - 1] = serial;
    heap[next_block->payload_index + cap - 3] = serial;
}


/*======================== list fit functions ======================= */

/* find free block based on first fit (cast to BLOCK_T* or ELIST_T*) */
void *find_first_fit(size_t size, char list_type)
{
    if(list_type == 'I') // procedure for implicit free list
    {
        if(head == NULL) return NULL;

        BLOCK_T *block = head;

        while(block != NULL)
        {
            if(block->free && block->size > size)
                return block;

            block = block->next;
        }

        return block;
    }
    if(list_type == 'E')  // procedure to explicit list
    {
        if(exp_list == NULL) return NULL;

        ELIST_T *hblock = exp_list;

        while(hblock != NULL)
        {
            if(hblock->block->size > size)
                return hblock;

            hblock = hblock->next;
        }

        return hblock;      
    }
}


/* find free block based on best fit (cast to BLOCK_T* or ELIST_T*) */
void *find_best_fit(size_t size, char list_type)
{
    if(list_type == 'I')
    {
        if(!head) return NULL;

        BLOCK_T *temp = head;
        BLOCK_T *block = NULL;

        while(temp != NULL)
        {
            if(temp->free && temp->size >= size)
            {
                if(block == NULL) block = temp;
                else
                {
                    if(temp->size - size < block->size - size)
                        block = temp;
                }
            }
            temp = temp->next;
        }

        return block;
    }

    if(list_type == 'E')
    {
        if(!exp_list) return NULL;

        ELIST_T *temp = exp_list;
        ELIST_T *hblock = NULL;

        while(temp != NULL)
        {
            if(temp->block->size >= size)
            {
                if(hblock == NULL) hblock = temp;
                else
                {
                    if((temp->block->size - size) < (hblock->block->size = size))
                        hblock = temp;
                }
            }
            temp = temp->next;
        }

        return hblock;
    }
}


/* merges free blocks into a contigous block of heap memory */
BLOCK_T *merge(ELIST_T *free_block, BLOCK_T *block, char *logic)
{
    // check double word alignment and adjust padding
    
    uint_t size = block->prev->size / sizeof(WORD);
    if(block->prev->size % sizeof(WORD) > 0) size++;

    while((size * sizeof(WORD)) % 8 > 0) size++;

    // adjust header footer size
    size += (2 + block->size / sizeof(WORD));
    if(block->size % sizeof(WORD) > 0) size++;

    if(block->next != NULL) 
        while((size * sizeof(WORD)) % 8 > 0) size++;

    block->prev->size = size * sizeof(WORD);  // update size

    /* updating links in the list used */

    block->prev->next = block->next;
    if(block->next != NULL) block->next->prev = block->prev;

    BLOCK_T *prev = block->prev;

    if(list_type == 'E')
    {
        ELIST_T *tmp = exp_list;

        while(tmp != NULL)
        {
            if(tmp->block == block->prev || tmp->block == block)
            {
                free_block = tmp;
                break;
            }
            tmp = tmp->next;
        }

        // affirm block coalescing and update links
        if(free_block != NULL)
        {
            free_block->block = block->prev;

            if(free_block->next != NULL)
            {
                if(free_block->next != NULL)
                    free_block->prev->next = free_block->next;
                else
                    free_block->prev->next = NULL;
            }
        // ensure correct coalescing order
        if(!strcmp(logic, "right"))
        {
            if(free_block->next != NULL)
            {
                if(free_block->next->next != NULL)
                {
                    ELIST_T *tmp = free_block->next;
                    free_block->next = free_block->next->next;
                    free_block->next->next->prev = free_block->prev;
                    free(tmp);
                }
                else
                {
                    free(free_block->next);
                    free_block->next = NULL;
                }
                
            }
        }

        // continue updating links wherever necessary
        if(free_block->next != NULL)
        {
            if(free_block->prev != NULL)
                free_block->next->prev = free_block->prev;
            else
            {
                free_block->next->prev = NULL;
            }
            
        }
        else if(free_block->next == NULL)
        {
            if(free_block->prev != NULL)
            {
                free_block->next = free_block->prev;
                free_block->prev->prev = free_block;
                free_block->prev = NULL;
            }
        }

        // attempt updating the double links
        if(free_block != exp_list)
        {
            free_block->next = exp_list;
            exp_list->prev = free_block;
            exp_list = free_block;
        }            
        }
        else
        {
            free_block = (ELIST_T *)malloc(sizeof(ELIST_T));
            free_block->block = prev;
            free_block->next = exp_list;
            free_block->prev = NULL;
            exp_list->prev = free_block;
            exp_list = free_block;
        }
    }   

    free(block);
    block = NULL;

    return prev;
}

BLOCK_T *coalesce(BLOCK_T *block)
{
    // coalesce the last block with the rest of the blocks
    if(block->next = NULL)
    {
        size_t size = block->size / sizeof(WORD);
        if(block->size % sizeof(WORD) > 0) size++;

        while((size * sizeof(WORD)) % 8 > 0) size++;

        if(block->payload_index + size < heap_size / sizeof(WORD))
        {
            size_t space = heap_size / sizeof(WORD) - block->payload_index;
            block->size = space * sizeof(WORD);
        }
    }

    ELIST_T *free_block = NULL;

    if(block->free)
    {
        int merged = 0;
        if(block->prev != NULL)
        {
            if(block->prev->free)
            {
                block = merge(free_block, block, "left");
                merged = 1;
            }
        }
        if(block->next != NULL)
        {
            if(block->next->free)
            {
                block = merge(free_block, block, "right");
                merged = 1;
            }
        }

        if(merged == 0)
        {
            if(list_type == 'E')
            {
                free_block = (ELIST_T *)malloc(sizeof(ELIST_T));
                free_block->block = block;
                free_block->prev = NULL;
                free_block->next = exp_list;
                exp_list->prev = free_block;
                exp_list = free_block;
            }
        }
    }

    // update the heap
    if(block->next != NULL)
    {
        size_t size = block->size / sizeof(WORD);
        if(block->size % sizeof(WORD) > 0) size++;

        while((size * sizeof(WORD)) % 8 > 0) size++;

        int serial = (size - 1) * 4;
        heap[block->payload_index - 1] = serial;
        heap[block->payload_index + size - 3] = serial;        
    }

    return block;
}

#endif