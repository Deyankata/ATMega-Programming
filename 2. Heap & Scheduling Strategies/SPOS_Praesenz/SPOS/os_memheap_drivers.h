
 /*
 * os_memheap_drivers.h
 *
 * Contains management of several logical heaps, each associated with one MemDriver.
 *
 * Created: 15.11.2023 23:06:54
 *  Author: bx541002
 */ 


#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_mem_drivers.h"
#include <stddef.h>

#define HEAP_START (SRAM_START + HEAPOFFSET)
#define HEAP_SIZE (SRAM_SIZE / 2 - HEAPOFFSET)
#define HEAP_MAPSTART (HEAP_START)
#define HEAP_MAPSIZE (HEAP_SIZE / 3)
#define HEAP_USESTART (HEAP_MAPSTART + HEAP_MAPSIZE)
#define HEAP_USESIZE (HEAP_MAPSIZE * 2)

// All available heap allocation strategies.
typedef enum AllocStrategy{
	OS_MEM_FIRST,
	OS_MEM_BEST,
	OS_MEM_WORST,
	OS_MEM_NEXT
} AllocStrategy;

/* The structure of a heap driver which consists of a low level memory driver and 
heap specific information such as start, size etc...
*/
typedef struct Heap{
	MemDriver *driver;
	MemAddr mapStart;
	uint16_t mapSize;
	MemAddr useStart;
	uint16_t useSize;
	AllocStrategy curAllocStrat;
	char const *name;
} Heap;

#define intHeap (&intHeap__)
extern Heap intHeap__;

// Initializes all Heaps.
void os_initHeaps(void);

// Returns the heap that corresponds to the given index (e.g intHeap if the index was 0)
Heap* os_lookupHeap(uint8_t index);

// The number of Heaps existing, e.g. 1 if there is only the intHeap
size_t os_getHeapListLength(void);

#endif /* OS_MEMHEAP_DRIVERS_H_ */
