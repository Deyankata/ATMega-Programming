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
ProcessID getOwnerOfChunk(const Heap *heap, MemAddr addr);
void moveChunk(Heap *heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize);

// Function used to allocate private memory
MemAddr os_malloc(Heap *heap, size_t size){
	os_enterCriticalSection();
	
	MemAddr curFrameStart =  heap->allocFrameStart[os_getCurrentProc()];
	MemAddr curFrameEnd = heap->allocFrameEnd[os_getCurrentProc()];
	
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
	
	// Adjust process frame positions
	// First allocation of the current process
	if (curFrameStart == 0 && curFrameEnd == 0){
		heap->allocFrameStart[os_getCurrentProc()] = startAddress;
		heap->allocFrameEnd[os_getCurrentProc()] = curAddress;
	}
	
	// Current process allocates memory before the current frame
	else if (startAddress < curFrameStart){
		heap->allocFrameStart[os_getCurrentProc()] = startAddress;
	}
	
	// Current process allocates memory after the current frame
	else if (curFrameEnd < curAddress){
		heap->allocFrameEnd[os_getCurrentProc()] = curAddress;
	}

	os_leaveCriticalSection();
	
	return startAddress;
}

MemAddr os_realloc(Heap *heap, MemAddr addr, uint16_t size){
	os_enterCriticalSection();
	
	MemAddr curFrameStart = heap->allocFrameStart[os_getCurrentProc()];
	MemAddr curFrameEnd = heap->allocFrameEnd[os_getCurrentProc()];
	
	uint16_t chunkSize = os_getChunkSize(heap, addr);
	MemAddr startOfChunk = os_getFirstByteOfChunk(heap, addr);
	MemAddr endOfChunk = startOfChunk + chunkSize;
	
	// A process tries to reallocate another's memory block
	if (getOwnerOfChunk(heap, addr) != os_getCurrentProc()){
		os_error("Different Owner");
		os_leaveCriticalSection();		
		return 0;
	}
	
	// A process tries to reallocate the same size, return addr
	if (chunkSize == size){
		os_leaveCriticalSection();
		return startOfChunk;
	}
	
	// A process wants to reduce its space
	if (size < chunkSize){
		MemAddr redundantAddr = startOfChunk + size;
		while (os_getMapEntry(heap, redundantAddr) == 0xF){
			os_setMapEntry(heap, redundantAddr, 0);
			heap->driver->write(redundantAddr, 0);
			redundantAddr++;
		}
		
		// If necessary, adjust allocFrameEnd to the correct position
		if (curFrameEnd == endOfChunk){
			heap->allocFrameEnd[os_getCurrentProc()] = endOfChunk - (chunkSize - size);
		}
		
		os_leaveCriticalSection();
		return startOfChunk;
	}
	
	// A process wants to increase its space
	if (size > chunkSize){
		// First, check if there is enough space to the right of the block
		MemAddr startSearchRight = endOfChunk;
		size_t additionalSpace = 0;
		while (os_getMapEntry(heap, startSearchRight) == 0){
			additionalSpace++;
			startSearchRight++;
			
			// Check if there is enough available space
			if (additionalSpace == size - chunkSize){
				// Fill in the additional space
				for (MemAddr curAddr = endOfChunk; curAddr < endOfChunk + additionalSpace; curAddr++){
					os_setMapEntry(heap, curAddr, 0xF);
				}
				
				// If necessary, adjust allocFrameEnd to the correct position
				if (curFrameEnd < startSearchRight){
					heap->allocFrameEnd[os_getCurrentProc()] = startSearchRight;
				}
				
				os_leaveCriticalSection();
				return startOfChunk;
			}
		}
		
		// If not, check if there is enough space to the left of the block
		MemAddr startSearchLeft = startOfChunk - 1;
		additionalSpace = 0; // Reset this variable
		while (os_getMapEntry(heap, startSearchLeft) == 0){
			additionalSpace++;
			startSearchLeft--;
		}
		
		// Check if there is enough available space
		if (additionalSpace >= size - chunkSize){
			// Move the chunk
			moveChunk(heap, startOfChunk, chunkSize, startOfChunk - additionalSpace, size);
			
			// If necessary, adjust allocFrameStart to the correct position
			if (curFrameStart > startSearchLeft){
				heap->allocFrameStart[os_getCurrentProc()] = startSearchLeft;
			}

			os_leaveCriticalSection();
			return startOfChunk - additionalSpace;
		}
		
		// Lastly, check if the sum of the free space both from the right and from the left is enough for the reallocation
		// Reset the following variables
		additionalSpace = 0; 
		startSearchRight = endOfChunk;
		startSearchLeft = startOfChunk - 1;
		// Find the size of the free space to the right
		while(os_getMapEntry(heap, startSearchRight) == 0){
			additionalSpace++;
			startSearchRight++;
		}
		// Find the size of the free space to the left
		while (os_getMapEntry(heap, startSearchLeft) == 0){
			additionalSpace++;
			startSearchLeft--;
		}
		// Check if there is enough available space. If yes, move the chunk to the left
		if (additionalSpace >= size - chunkSize){
			// Move chunk
			moveChunk(heap, startOfChunk, chunkSize, startSearchLeft + 1, size);
			
			// If necessary, adjust allocFrameStart and allocFrameEnd to the correct position
			if (curFrameStart > startSearchLeft){
				heap->allocFrameStart[os_getCurrentProc()] = startSearchLeft;
			}
			else if (curFrameEnd == endOfChunk){
				heap->allocFrameEnd[os_getCurrentProc()] = startSearchLeft + size;
			}
			
			os_leaveCriticalSection();
			return startSearchLeft + 1;
		}
		
		// If none of the cases above are applicable, try to allocate a new memory block
		MemAddr newStartAddress = os_malloc(heap, size);
		if (newStartAddress != 0){
			moveChunk(heap, startOfChunk, chunkSize, newStartAddress, size);
			os_free(heap, addr);
			
			os_leaveCriticalSection();
			return newStartAddress;	
		}
	}
	
	os_leaveCriticalSection();
	return 0;
}

MemAddr os_sh_malloc(Heap *heap, size_t size){
	os_enterCriticalSection();
	
	MemAddr startAddress = 0;
	AllocStrategy strategy = os_getAllocationStrategy(heap);
	
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
		return startAddress;
	}
	
	// Set the first value in the allocation map to the process requesting the space
	os_setMapEntry(heap, startAddress, SH_MEMORY_ID);
	
	// Set the rest of the requested space to F (as in the protocol)
	MemAddr curAddress = startAddress + 1;
	for (MemAddr i = 1; i < size; i++){
		os_setMapEntry(heap, curAddress, 0xF);
		curAddress++;
	}
	
	os_leaveCriticalSection();
	
	return startAddress;
}

// Function used by processes to free their own allocated memory 
void os_free(Heap *heap, MemAddr addr){
	os_enterCriticalSection();
	
	MemAddr curFrameStart =  heap->allocFrameStart[os_getCurrentProc()];
	MemAddr curFrameEnd = heap->allocFrameEnd[os_getCurrentProc()];
	
	MemAddr startAddress = 0;
	
	// Check if address is the start of the memory block. If not, go to the start
	startAddress = os_getFirstByteOfChunk(heap, addr);
	
	// A process tries to free shared memory
	if (os_getMapEntry(heap, startAddress) >= SH_MEMORY_ID){
		os_error("Shared Memory");
		os_leaveCriticalSection();
		return;
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
	
	// Adjust the process frame to the correct positions
	// If the whole frame is being removed
	if (curFrameStart == startAddress && curFrameEnd == curAddress){
		heap->allocFrameStart[os_getCurrentProc()] = 0;
		heap->allocFrameEnd[os_getCurrentProc()] = 0;
	}
	
	// If the left most block if the frame is being removed
	else if (curFrameStart == startAddress){
		while (os_getMapEntry(heap, curAddress) != os_getCurrentProc()){
			curAddress++;
		}
		
		heap->allocFrameStart[os_getCurrentProc()] = curAddress;
	}
	
	// If the right most block of the frame is being removed
	else if (curFrameEnd == curAddress){
		while (os_getMapEntry(heap, startAddress) != os_getCurrentProc()){
			startAddress--;
		}
		
		heap->allocFrameEnd[os_getCurrentProc()] = startAddress + os_getChunkSize(heap, startAddress);
	}
	
	os_leaveCriticalSection();
}

void os_sh_free(Heap *heap, MemAddr *addr){
	os_enterCriticalSection();
	
	MemAddr firstAddress = os_getFirstByteOfChunk(heap, *addr);
	
	// Process block is not shared 
	if (os_getMapEntry(heap, firstAddress) < SH_MEMORY_ID){
		os_error("ERROR:Shared Mem");
		return;
	}
	
	// Wait for the shared memory to close 
	while (os_getMapEntry(heap, firstAddress) != SH_MEMORY_ID){
		os_yield();
		firstAddress = os_getFirstByteOfChunk(heap, *addr);
	}
	
	// Set the first address to 0 (as in the protocol)
	os_setMapEntry(heap, firstAddress, 0);
	
	// Set the rest to 0
	MemAddr curAddress = firstAddress + 1;
	while (os_getMapEntry(heap, curAddress) == 0xF && curAddress < os_getUseStart(heap) + os_getUseSize(heap)){
		os_setMapEntry(heap, curAddress, 0);
		curAddress++;
	}
	
	os_leaveCriticalSection();
}

MemAddr os_sh_readOpen(const Heap *heap, const MemAddr *ptr){
	os_enterCriticalSection();
	
	MemAddr firstAddress = os_getFirstByteOfChunk(heap, *ptr);
	
	// A process tries to access a non-shared memory block
	if (os_getMapEntry(heap, firstAddress) < SH_MEMORY_ID){
		os_error("ERROR:Shared Mem");
		os_leaveCriticalSection();
		return 0;
	}
	
	// Other operations are currently being executed, wait for them to finish
	while (os_getMapEntry(heap, firstAddress) > SH_MEMORY_READ_P1){
		os_yield();
		firstAddress = os_getFirstByteOfChunk(heap, *ptr);
	}
	
	if (os_getMapEntry(heap, firstAddress) == SH_MEMORY_ID){
		// Set READ flag for shared memory
		os_setMapEntry(heap, firstAddress, SH_MEMORY_READ_P1);
	}
	// If one read operation is currently active, activate a second one
	else if (os_getMapEntry(heap, firstAddress) == SH_MEMORY_READ_P1){
		os_setMapEntry(heap, firstAddress, SH_MEMORY_READ_P2);
	}
	else {
		os_leaveCriticalSection();
		return 0;
	}
	
	os_leaveCriticalSection();
	return firstAddress;
}

MemAddr os_sh_writeOpen(const Heap *heap, const MemAddr *ptr){
	os_enterCriticalSection();
	
	MemAddr firstAddress = os_getFirstByteOfChunk(heap, *ptr);
	
	// A process tries to access a non-shared memory block
	if (os_getMapEntry(heap, firstAddress) < SH_MEMORY_ID){	
		os_error("ERROR:Shared Mem");
		os_leaveCriticalSection();
		return 0;
	}
	
	// Other operations are currently being executed, wait for them to finish
	while (os_getMapEntry(heap, firstAddress) != SH_MEMORY_ID){
		os_yield();
		firstAddress = os_getFirstByteOfChunk(heap, *ptr);
	}
	
	// Set shared memory with WRITE flag
	if (os_getMapEntry(heap, firstAddress) == SH_MEMORY_ID){
		os_setMapEntry(heap, firstAddress, SH_MEMORY_WRITE);
		
		os_leaveCriticalSection();
		return firstAddress;
	}
	
	os_leaveCriticalSection();
	return 0;
}

void os_sh_close(const Heap *heap, MemAddr addr){
	os_enterCriticalSection();
	
	MemAddr firstAddress = os_getFirstByteOfChunk(heap, addr);
	
	if (os_getMapEntry(heap, firstAddress) < SH_MEMORY_ID){
		os_error("ERROR:Shared Mem");
	}

	// Close the shared memory flag respectively
	uint16_t flag = os_getMapEntry(heap, firstAddress);
	switch (flag){
		case SH_MEMORY_READ_P1:
			os_setMapEntry(heap, firstAddress, SH_MEMORY_ID);
			break;
		case SH_MEMORY_READ_P2:
			os_setMapEntry(heap, addr, SH_MEMORY_READ_P1);
			break;
		case SH_MEMORY_WRITE:
			os_setMapEntry(heap, firstAddress, SH_MEMORY_ID);
			break;
		default:
			break;
	}
	
	os_leaveCriticalSection();
}


void os_sh_read(const Heap *heap, const MemAddr *ptr, uint16_t offset, MemValue *dataDest, uint16_t length){
	MemAddr readAddress = os_sh_readOpen(heap, ptr);
	
	if (offset + length > os_getChunkSize(heap, readAddress)){
		os_error("Size invalid");
		os_sh_close(heap, readAddress);
		return;
	}
	
	// Read from the shared memory and copy the data to dataDest
	for (uint16_t index = 0; index < length; index++){
		dataDest[index] = heap->driver->read(readAddress + offset + index);
	}
	
	os_sh_close(heap, readAddress);
}

void os_sh_write(const Heap *heap, const MemAddr *ptr, uint16_t offset, const MemValue *dataSrc, uint16_t length){
	MemAddr writeAddress = os_sh_writeOpen(heap, ptr);
	
	if (offset + length > os_getChunkSize(heap, writeAddress)){
		os_error("Size invalid");
		os_sh_close(heap, writeAddress);
		return;
	}
	
	// Read data from dataSrc and write it to the shared memory
	for (uint16_t index = 0; index < length; index++){
		heap->driver->write(writeAddress + offset + index, dataSrc[index]);
	}
	
	os_sh_close(heap, writeAddress);
}

// Writes a value from 0x0 to 0xF to the lower nibble of the given address
void setLowNibble(const Heap *heap, MemAddr addr, MemValue value){
	MemValue lowNibble = value | getHighNibble(heap, addr);
	heap->driver->write(addr, lowNibble);
}

// Writes a value from 0x0 to 0xF to the higher nibble of the given address
void setHighNibble(const Heap *heap, MemAddr addr, MemValue value){
	MemValue highNibble = (value << 4) | getLowNibble(heap, addr);
	heap->driver->write(addr, highNibble);
}

// Reads the value of the lower nibble of the given address
MemValue getLowNibble(const Heap *heap, MemAddr addr){
	return (heap->driver->read(addr) & 0b00001111);
}

// Reads the value of the higher nibble of the given address
MemValue getHighNibble(const Heap *heap, MemAddr addr){
	return (heap->driver->read(addr) & 0b11110000);
}

// This function is used to set a heap map entry on a specific heap
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

// Function used to get the value of a single map entry, this is made public so the allocation strategies can use it
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

// Get the size of the heap-map
size_t os_getMapSize(const Heap *heap){
	return heap->mapSize;
}

// Get the size of the usable heap
size_t os_getUseSize(const Heap *heap){
	return heap->useSize;
}

// Get the start of the heap-map
MemAddr os_getMapStart(const Heap *heap){
	return heap->mapStart;
}

// Get the start of the usable heap
MemAddr os_getUseStart(const Heap *heap){
	return heap->useStart;
}
 
// Get the size of a chunk on a given address
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
			addr++;
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
	
	// Return 0 if for some reason none of the cases above return a value
	return 0;
}
 
// Get the first byte of a chunk
MemAddr os_getFirstByteOfChunk(const Heap *heap, MemAddr addr){
	while (os_getMapEntry(heap, addr) == 0xF){
		addr--;
	}
	
	return addr;
}
 
// Move chunk to a new location
void moveChunk(Heap *heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize){
	ProcessID currentID = heap->driver->read(oldChunk);
	// Check if the chunk wants to move to the left
	if (newChunk < oldChunk){
		uint16_t count = 0;
		for (MemAddr curAddr = newChunk; curAddr < newChunk + newSize; curAddr++){
			// Move the current block to the left
			if (count < oldSize){
				os_setMapEntry(heap, curAddr, os_getMapEntry(heap, oldChunk + count));
				heap->driver->write(curAddr, currentID);
				os_setMapEntry(heap, oldChunk + count, 0);
				heap->driver->write(oldChunk + count, 0);
				count++;
			}
			// Fill in the additional space
			else{			
				os_setMapEntry(heap, curAddr, 0xF);
				heap->driver->write(curAddr, currentID);
			}
		}
	}
}
 
// This function determines the value of the first nibble of a chunk.
ProcessID getOwnerOfChunk(const Heap *heap, MemAddr addr){
	return os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
}
 
// Changes the memory management strategy
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->curAllocStrat = allocStrat;
}
 
// Returns the current memory management strategy
AllocStrategy os_getAllocationStrategy(const Heap *heap){
	return heap->curAllocStrat;
}
 
// Function that realizes the garbage collection
void os_freeProcessMemory(Heap *heap, ProcessID pid){	 
	for (MemAddr curAddr = heap->allocFrameStart[pid]; curAddr <= heap->allocFrameEnd[pid]; curAddr++){
		if (os_getMapEntry(heap, curAddr) == pid){
			os_free(heap, curAddr);
		}
	}
}
	 