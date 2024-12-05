/*
 * os_memory.c
 *
 * Contains functions to allocate blocks of any spos-memory-drivers. 
 * We manage the blocks by creating a lookup table with one nibble per byte payload. 
 * So we have 1/3 map and 2/3 use-heap of each driver.
 *
 * Created: 15.11.2023 23:37:32
 *  Author: bx541002
 */ 

#include "os_memory.h"
#include "os_core.h"
#include "os_memory_strategies.h"
#include "util.h"
#include "lcd.h"

// Function prototypes
MemValue getLowNibble(const Heap *heap, MemAddr addr);
MemValue getHighNibble(const Heap *heap, MemAddr addr);
void os_setMapEntry(const Heap *heap, MemAddr addr, MemValue value);

// Function used to allocate private memory.
MemAddr os_malloc(Heap *heap, size_t size){
	os_enterCriticalSection();
	
	AllocStrategy strategy = os_getAllocationStrategy(heap);
	MemAddr startAddress = 0;
	
	switch(strategy){
		case OS_MEM_FIRST:
			startAddress = os_Memory_FirstFit(heap, size);
			break;
		case OS_MEM_BEST:
			startAddress = os_Memory_BestFit(heap, size);
			break;
		case OS_MEM_WORST:
			startAddress = os_Memory_WorstFit(heap, size);
			break;
		case OS_MEM_NEXT:
			startAddress = os_Memory_NextFit(heap, size);
			break;
		default:
			break;
	}
	
	// Not enough available space
	if (startAddress == 0){
		os_leaveCriticalSection();
		return 0;
	}
	
	// Set the first value in the allocation map to the process requesting the space
	os_setMapEntry(heap, startAddress, os_getCurrentProc());
	
	// Set the rest of the requested space to F (as in the protocol)
	MemAddr curAddress = startAddress + 1;
	for (MemAddr i = 1; i < size; i++){
		os_setMapEntry(heap, curAddress, 0xF);
		curAddress++;
	}
	
	os_leaveCriticalSection();
	
	return startAddress;
}

// Function used by processes to free their own allocated memory. 
void os_free(Heap *heap, MemAddr addr){
	os_enterCriticalSection();
	
	MemAddr startAddress = 0;
	
	// Check if address is the start of the memory block. If not, go to the start
	if (os_getMapEntry(heap, addr) != 0xF){
		startAddress = addr;
	}
	else{
		while (os_getMapEntry(heap, addr) == 0xF){
			addr--;
		}
		startAddress = addr;
	}
	
	
	// Set the first address to 0 (as in the protocol)
	os_setMapEntry(heap, startAddress, 0);
	heap->driver->write(startAddress, 0);
	
	MemAddr curAddress = startAddress + 1;
	while (os_getMapEntry(heap, curAddress) == 0xF && curAddress < os_getUseStart(heap) + os_getUseSize(heap)){
		os_setMapEntry(heap, curAddress, 0);
		heap->driver->write(curAddress, 0);
		curAddress++;
	}
	
	os_leaveCriticalSection();
}

// Writes a value from 0x0 to 0xF to the lower nibble of the given address.
void setLowNibble(const Heap *heap, MemAddr addr, MemValue value){
	MemValue lowNibble = value | getHighNibble(heap, addr);
	heap->driver->write(addr, lowNibble);
}

// Writes a value from 0x0 to 0xF to the higher nibble of the given address.
void setHighNibble(const Heap *heap, MemAddr addr, MemValue value){
	MemValue highNibble = (value << 4) | getLowNibble(heap, addr);
	heap->driver->write(addr, highNibble);
}

// Reads the value of the lower nibble of the given address.
MemValue getLowNibble(const Heap *heap, MemAddr addr){
	return (heap->driver->read(addr) & 0b00001111);
}

// Reads the value of the higher nibble of the given address.
MemValue getHighNibble(const Heap *heap, MemAddr addr){
	return (heap->driver->read(addr) & 0b11110000);
}

// This function is used to set a heap map entry on a specific heap.
void os_setMapEntry(const Heap *heap, MemAddr addr, MemValue value){
	// Distance between the start of the user data and addr
	MemAddr useDistance = addr - os_getUseStart(heap);
	
	if (useDistance % 2 == 0){
		setHighNibble(heap, os_getMapStart(heap) + (useDistance / 2), value);
	}
	else{
		setLowNibble(heap, os_getMapStart(heap) + (useDistance / 2), value);
	}
}

// Function used to get the value of a single map entry, this is made public so the allocation strategies can use it.
MemValue os_getMapEntry(const Heap *heap, MemAddr addr){
	// Distance between the start of the user data and addr
	MemAddr useDistance = addr - os_getUseStart(heap);
	
	if (useDistance % 2 == 0){
		return getHighNibble(heap, os_getMapStart(heap) + (useDistance / 2)) >> 4;
	}
	else {
		return getLowNibble(heap, os_getMapStart(heap) + (useDistance / 2));
	}
}

// Get the size of the heap-map.
size_t os_getMapSize(const Heap *heap){
	return heap->mapSize;
}

// Get the size of the usable heap.
size_t os_getUseSize(const Heap *heap){
	return heap->useSize;
}

// Get the start of the heap-map.
MemAddr os_getMapStart(const Heap *heap){
	return heap->mapStart;
}

 // Get the start of the usable heap.
 MemAddr os_getUseStart(const Heap *heap){
	return heap->useStart;
 }
 
 // Get the size of a chunk on a given address.
 uint16_t os_getChunkSize(const Heap *heap, MemAddr addr){
	uint16_t chunkSize = 0;
	
	// The allocation map entry is free
	if(os_getMapEntry(heap, addr) == 0){
		return 0;
	}
	// The allocation map entry is a PID
	if (os_getMapEntry(heap, addr) != 0 && os_getMapEntry(heap, addr) != 0xF){
		chunkSize++;
		addr++;
		while (os_getMapEntry(heap, addr) == 0xF){
			chunkSize++;
		}
		return chunkSize;
	}
	// The allocation map entry is somewhere inside of the chunk
	if (os_getMapEntry(heap, addr) == 0xF){
		MemAddr addrRight = addr;
		MemAddr addrLeft = addr - 1;
		// Go to start of chunk
		while (os_getMapEntry(heap, addrLeft) == 0xF){
			addrLeft--;
			chunkSize++;
		}
		// Go to end of chunk
		while (os_getMapEntry(heap, addrRight) == 0xF){
			addrRight++;
			chunkSize++;
		}
		
		return chunkSize;
	}
	
	// Return if for some reason none of the cases above return a value
	return 0;
 }
 
 // Changes the memory management strategy.
 void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	 heap->curAllocStrat = allocStrat;
 }
 
 // Returns the current memory management strategy.
 AllocStrategy os_getAllocationStrategy(const Heap *heap){
	 return heap->curAllocStrat;
 }
 
 // Function that realizes the garbage collection. 
 void os_freeProcessMemeory(Heap *heap, ProcessID pid){	 
	for (MemAddr curAddr = heap->useStart; curAddr <= heap->useStart + heap->useSize; curAddr++){
		if (os_getMapEntry(heap, curAddr) == pid){
			os_free(heap, curAddr);
		}
	}
 }
	 