
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

#include "defines.h"
#include "os_mem_drivers.h"
#include <stddef.h>

#define INTERNAL_HEAP_START (INTERNAL_SRAM_START + HEAPOFFSET)
#define INTERNAL_HEAP_SIZE (INTERNAL_SRAM_SIZE / 2 - HEAPOFFSET)
#define INTERNAL_HEAP_MAPSTART (INTERNAL_HEAP_START)
#define INTERNAL_HEAP_MAPSIZE ((INTERNAL_HEAP_SIZE - 1) / 3) 
#define INTERNAL_HEAP_USESTART (INTERNAL_HEAP_MAPSTART + INTERNAL_HEAP_MAPSIZE)
#define INTERNAL_HEAP_USESIZE (INTERNAL_HEAP_MAPSIZE * 2)

#define EXTERNAL_HEAP_MAPSTART (EXTERNAL_SRAM_START)
#define EXTERNAL_HEAP_MAPSIZE (EXTERNAL_SRAM_SIZE / 3)
#define EXTERNAL_HEAP_USESTART (EXTERNAL_HEAP_MAPSTART + EXTERNAL_HEAP_MAPSIZE) 
#define EXTERNAL_HEAP_USESIZE (EXTERNAL_HEAP_MAPSIZE * 2)

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
	MemAddr allocFrameStart[MAX_NUMBER_OF_PROCESSES];
	MemAddr allocFrameEnd[MAX_NUMBER_OF_PROCESSES];
	char const *name;
} Heap;

extern Heap intHeap__;
#define intHeap (&intHeap__)

extern Heap extHeap__;
#define extHeap (&extHeap__)

// Initializes all Heaps.
void os_initHeaps(void);

// Returns the heap that corresponds to the given index (e.g intHeap if the index was 0)
Heap* os_lookupHeap(uint8_t index);

// The number of Heaps existing, e.g. 1 if there is only the intHeap
size_t os_getHeapListLength(void);

#endif /* OS_MEMHEAP_DRIVERS_H_ */
