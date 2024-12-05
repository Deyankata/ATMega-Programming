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

#define SH_MEMORY_ID 8
#define SH_MEMORY_READ_P1 9
#define SH_MEMORY_READ_P2 10
#define SH_MEMORY_WRITE 11

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"

// Function used to allocate private memory
MemAddr os_malloc(Heap *heap, size_t size);

// Function used to allocate shared memory
MemAddr os_sh_malloc(Heap *heap, size_t size);

// Function to efficiently reallocate memory
MemAddr os_realloc(Heap *heap, MemAddr addr, uint16_t size);

// Function used by processes to free their own allocated memory
void os_free(Heap *heap, MemAddr addr);

// Function used by processes to free shared memory
void os_sh_free(Heap *heap, MemAddr *addr);

// Function used to write to shared memory
void os_sh_write(const Heap *heap, const MemAddr *ptr, uint16_t offset, const MemValue *dataSrc, uint16_t length);

// Function used to read from shared memory
void os_sh_read(const Heap *heap, const MemAddr *ptr, uint16_t offset, MemValue *dataDest, uint16_t length);

// Function that should be private but is used by some Test tasks
MemAddr os_sh_readOpen(const Heap *heap, const MemAddr *ptr);

// Function that should be private but is used by some Test tasks
MemAddr os_sh_writeOpen(const Heap *heap, const MemAddr *ptr);

// Function that should be private but is used by some Test tasks
void os_sh_close(const Heap *heap, MemAddr addr);

// Function used to get the value of a single map entry, this is made public so the allocation strategies can use it
MemValue os_getMapEntry(const Heap *heap, MemAddr addr);

// Get the size of the heap-map
size_t os_getMapSize(const Heap *heap);

// Get the size of the usable heap
size_t os_getUseSize(const Heap *heap);

// Get the start of the heap-map
MemAddr os_getMapStart(const Heap *heap);

// Get the start of the usable heap
MemAddr os_getUseStart(const Heap *heap);

// Get the size of a chunk on a given address
uint16_t os_getChunkSize(const Heap *heap, MemAddr addr);

// Get the address of the first byte of chunk
MemAddr os_getFirstByteOfChunk(const Heap *heap, MemAddr addr);

// Changes the memory management strategy
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

// Returns the current memory management strategy
AllocStrategy os_getAllocationStrategy(const Heap *heap);

// Function that realizes the garbage collection
void os_freeProcessMemory(Heap *heap, ProcessID pid);

#endif /* OS_MEMORY_H_ */