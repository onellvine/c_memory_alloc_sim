#include <stdio.h>
#include <stdbool.h>

#include "List.h"

void mysbrk(size_t size)
{
    if(heap_size + size + 8 > MAX_HEAP)
    {
        char *file_name = "output.txt";
        FILE *fp = fopen(file_name, "W+");
        fprintf(fp, "Error: Heap grew past 100000 WORDS.\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    heap_size += (size + 8);

    return;
}

void *myalloc(size_t size)
{
    BLOCK_T *block = NULL;

    if(list_type == 'I')
    {
        if(fit_type == 'F') block = find_first_fit(size, 'I');
        else if(fit_type == 'B')
            block = find_best_fit(size, 'I');
    }

    ELIST_T * free_block = NULL;

    if(list_type == 'E')
    {
        if(fit_type == 'F')
        {
            free_block = find_first_fit(size, 'E');
            if(free_block != NULL)
                block = free_block->block;
        }
        else if(fit_type == 'B')
        {
            free_block = find_best_fit(size, 'E');
            if(free_block != NULL)
                block = free_block->block;
        }
    }

    // check enough space for the block
    if(block == NULL && head != NULL)
    {
        BLOCK_T *temp = head;
        block = temp;

        while(true)
        {
            temp = temp->next;
            if(temp == NULL) break;
            block = temp;
        }

        mysbrk(size);

        BLOCK_T *nfree_block = malloc(sizeof(BLOCK_T));
        nfree_block->size = size;
        nfree_block->free = true;
        nfree_block->payload_index = heap_size / sizeof(WORD) - size / sizeof(WORD) - 2;
        nfree_block->prev = block;
        block->next = nfree_block;
        nfree_block->next = NULL;
        block = nfree_block;

        if(list_type == 'E')
        {
            free_block = malloc(sizeof(ELIST_T));
            free_block->next = exp_list;
            free_block->prev = NULL;
            exp_list = free_block;
        }
    }

    if(block == NULL)
    {
        block = malloc(sizeof(BLOCK_T));
        block->size = size;
        block->next = NULL;
        block->free = false;

        if(head == NULL)
        {
            block->payload_index = 2;
            block->prev = NULL;
            head = block;
        }
    }
    else
    {
        if(block->size - size > 2)
            split(block, size, free_block);
        else
        {
            block->size = size;
            block->free = true;

            if(list_type == 'E')
            {
                if(free_block->prev != NULL)
                {
                    if(free_block->next != NULL)
                        free_block->prev->next = free_block->next;
                    else
                        free_block->prev->next = NULL;
                }
                if(free_block->next != NULL)
                {
                    if(free_block->prev !=  NULL)
                        free_block->next->prev = free_block->prev;
                    else
                        free_block->next->prev = NULL;
                }

                // update the head of the list
                if(free_block == exp_list)
                {
                    if(free_block->next != NULL)
                        exp_list = free_block->next;
                    else
                        exp_list = NULL;
                }
                free(free_block);
            }
        }
    }

    // update the heap   
    size_t cap = block->size / sizeof(WORD);
    if(block->size % sizeof(WORD) > 0) cap++;

    while((size * sizeof(WORD)) % 8 > 0) cap++;

    int serial = (cap - 1) * 4;
    heap[block->payload_index - 1] = serial;
    heap[block->payload_index + cap - 3] = serial;

    return &block->payload_index;
}


void myfree(void *ptr)
{
    BLOCK_T *block = ptr;
    block->free = true;

    coalesce(block);

    return;
}


void *myrealloc(void *ptr, size_t size)
{
    void *p = myalloc(size);
    BLOCK_T *block1 = p;
    BLOCK_T *block2 = ptr;

    int cap = block2->size / sizeof(WORD);
    if(block2->size % sizeof(WORD) > 0) cap++;

    while((cap * sizeof(WORD)) % 8 > 0) cap++;

    for(int i = 0; i < cap; i++)
        heap[block1->payload_index + i] = heap[block2->payload_index + i];

    myfree(ptr);

    return p;
}
