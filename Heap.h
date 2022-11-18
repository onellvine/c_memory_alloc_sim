#ifndef _HEAP_H_
#define _HEAP_H_

#include <unistd.h>


#define WORD unsigned int
#define MAX_HEAP 100000    // heap may expand to 100000 words maximum

unsigned int heap[MAX_HEAP];
size_t heap_size = 1000 * sizeof(WORD); // initial heap size is 1000 words


#endif