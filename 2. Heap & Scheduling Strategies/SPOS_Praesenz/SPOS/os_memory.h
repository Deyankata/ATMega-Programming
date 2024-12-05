/*
 * os_memory.h
 *
 * Contains heap management functionality for the OS.
 *
 * Created: 15.11.2023 23:37:53
 *  Author: bx541002
 */ 


#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"

// Function used to allocate private memory.
MemAddr os_malloc(Heap *heap, size_t size);

// Function used by processes to free their own allocated memory.
void os_free(Heap *heap, MemAddr addr);

// Function used to get the value of a single map entry, this is made public so the allocation strategies can use it.
MemValue os_getMapEntry(const Heap *heap, MemAddr addr);

// Get the size of the heap-map.
size_t os_getMapSize(const Heap *heap);

// Get the size of the usable heap. 
size_t os_getUseSize(const Heap *heap);

// Get the start of the heap-map.
MemAddr os_getMapStart(const Heap *heap);

// Get the start of the usable heap.
MemAddr os_getUseStart(const Heap *heap);

// Get the size of a chunk on a given address.
uint16_t os_getChunkSize(const Heap *heap, MemAddr addr);

// Changes the memory management strategy.
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

// Returns the current memory management strategy.
AllocStrategy os_getAllocationStrategy(const Heap *heap);

// Function that realises the garbage collection.
void os_freeProcessMemeory(Heap *heap, ProcessID pid);

#endif /* OS_MEMORY_H_ */