/*
 * os_memheap_drivers.c
 *
 * Heap Driver structs and some initialisation functions.
 * Some small functions needed by the Taskmanager are also placed here
 *
 * Created: 15.11.2023 23:07:13
 *  Author: bx541002
 */

#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_memory_strategies.h"
#include <avr/pgmspace.h>

static char const PROGMEM heapName[] = "intHeap";

Heap intHeap__ = {
	.driver = intSRAM,
	.mapStart = HEAP_MAPSTART,		// Add offset to avoid conflicts with global variables
	.mapSize = HEAP_MAPSIZE,		// SRAM size = 4096 => (4096 / 2 - HEAPOFFSET) * (1/3) = size of map (1/3th of the heap memory)
	.useStart = HEAP_USESTART,
	.useSize = HEAP_USESIZE,		// (4096 / 2 - HEAPOFFSET) * (2/3) = size of use (2/3ths of the heap memory)
	.curAllocStrat = OS_MEM_FIRST,
	.name = heapName
};
// Function that initializes all Heaps of the OS.
void os_initHeaps(){
	intHeap__.driver->init();
	// Iterate over the map area and set each nibble to 0h
	for (MemAddr addr = intHeap__.mapStart; addr < intHeap__.mapStart + intHeap__.mapSize; addr++){
		intHeap__.driver->write(addr, 0);
	}
}

// Returns the heap that corresponds to the given index (e.g intHeap if the index was 0)
Heap* os_lookupHeap(uint8_t index){
	if (index == 0){
		return intHeap;
	}
	// Handle other heap indices if applicable in the future
	return 0;
}

// Returns the number of Heaps.
size_t os_getHeapListLength(){
	return 1; // Only the internal heap is available (for now)
}