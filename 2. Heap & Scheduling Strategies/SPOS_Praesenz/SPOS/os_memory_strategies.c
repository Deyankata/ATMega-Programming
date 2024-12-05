/*
 * os_memory_strategies.c
 *
 * Memory strategies used by the malloc function of os_memory to allocate blocks on the memory.
 *
 * The file contains four strategies: -First-Fit -Next-Fit -Best-Fit -Worst-Fit
 *
 * Created: 15.11.2023 23:11:32
 *  Author: bx541002
 */

#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr nextFitLastAddr = 0;

MemAddr os_Memory_FirstFit(Heap *heap, size_t size){
	MemAddr currentAddr = os_getUseStart(heap);
	
	while (currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
		size_t blockSize = 0;
		
		// Find the size of the free block starting from currentAddr
		while (os_getMapEntry(heap, currentAddr) == 0 && currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
			blockSize++;
			currentAddr++;
		}
		
		// Check if the block is large enough for the allocation
		if (blockSize >= size){
			return currentAddr - blockSize; // Start address of the block
		}
		
		currentAddr++;
	}

	// If no suitable block is found, return 0
	return 0;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size){
	// Check if size is valid
	if (size > os_getUseSize(heap)){
		return 0;
	}
	
	MemAddr bestFitAddr = 0;
	size_t bestFitSize = os_getUseSize(heap); 
	
	MemAddr currentAddr = os_getUseStart(heap);
	
	while (currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
		size_t blockSize = 0;
		
		// Find the size of the free block starting from currentAddr
		while (os_getMapEntry(heap, currentAddr) == 0 && currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
			blockSize++;
			currentAddr++;
		}
		
		// Check if the block is large enough for the allocation and smaller than the current best fit
		if (blockSize >= size && blockSize < bestFitSize){
			bestFitSize = blockSize;
			bestFitAddr = currentAddr - blockSize; // Start address of the block
		}
		
		currentAddr++;
	}
	
	// If no suitable block is found, check if request for max size is at hand
	if (bestFitSize == os_getUseSize(heap)){
		return os_getUseStart(heap);
	}
	
	return bestFitAddr;
} 

MemAddr os_Memory_WorstFit(Heap *heap, size_t size){
	MemAddr worstFitAddr = 0;
	size_t worstFitSize = 0;
	
	MemAddr currentAddr = os_getUseStart(heap);
	
	while (currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
		size_t blockSize = 0;
		
		// Find the size of the free block starting from currentAddr
		while (os_getMapEntry(heap, currentAddr) == 0 && currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
			blockSize++;
			currentAddr++;
		}
		
		// Check if the block is large enough for the allocation and larger than the current worst fit
		if (blockSize >= size && blockSize > worstFitSize){
			worstFitSize = blockSize;
			worstFitAddr = currentAddr - blockSize; // Start address of the worst fit block
		}
		
		currentAddr++;
	}
	
	 // If no suitable block is found, return 0
	 if (worstFitSize == 0) {
		 return 0;
	 }
	 
	 return worstFitAddr;
}

MemAddr os_Memory_NextFit(Heap *heap, size_t size){
	MemAddr currentAddr = nextFitLastAddr;
	
	// If the last allocated address is not valid, start from the beginning
	if (currentAddr < os_getUseStart(heap) || currentAddr >= os_getUseStart(heap) + os_getUseSize(heap)){
		currentAddr = os_getUseStart(heap);
	}
	
	while (currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
		size_t blockSize = 0;
		
		while (os_getMapEntry(heap, currentAddr) == 0 && currentAddr < os_getUseStart(heap) + os_getUseSize(heap)){
			blockSize++;
			currentAddr++;
		}
		
		// Check if the block is large enough for the allocation
		if (blockSize >= size){
			// Update the last allocated address
			nextFitLastAddr = currentAddr;
			
			return currentAddr - blockSize; // Start address of the best fit block
		}
		
		currentAddr++;
	}
	
	// If no suitable block is found, return 0
	return 0;
}