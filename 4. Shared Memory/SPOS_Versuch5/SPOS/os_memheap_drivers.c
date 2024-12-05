/*
 * os_memheap_drivers.c
 *
 * Heap Driver structs and some initialization functions.
 * Some small functions needed by the Taskmanager are also placed here
 *
 * Created: 15.11.2023 23:07:13
 *  Author: bx541002
 */

#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_memory_strategies.h"
#include <avr/pgmspace.h>

static char const PROGMEM intHeapName[] = "intHeap";
static char const PROGMEM extHeapName[] = "extHeap";

Heap intHeap__ = {
	.driver = intSRAM,
	.mapStart = INTERNAL_HEAP_MAPSTART,		
	.mapSize = INTERNAL_HEAP_MAPSIZE,		
	.useStart = INTERNAL_HEAP_USESTART,
	.useSize = INTERNAL_HEAP_USESIZE,		
	.curAllocStrat = OS_MEM_FIRST,
	.name = intHeapName
};

Heap extHeap__ = {
	.driver = extSRAM,
	.mapStart = EXTERNAL_HEAP_MAPSTART,
	.mapSize = EXTERNAL_HEAP_MAPSIZE,
	.useStart = EXTERNAL_HEAP_USESTART,
	.useSize = EXTERNAL_HEAP_USESIZE,
	.curAllocStrat = OS_MEM_FIRST,
	.name = extHeapName	
};

// Function that initializes all Heaps of the OS.
void os_initHeaps(){
	intHeap__.driver->init();
	extHeap__.driver->init();
	// Iterate over the map area and set each nibble to 0h
	for (MemAddr addr = intHeap__.mapStart; addr < intHeap__.mapStart + intHeap__.mapSize; addr++){
		intHeap__.driver->write(addr, 0);
	}
	
	for (MemAddr addr = extHeap__.mapStart; addr < extHeap__.mapStart + extHeap__.mapSize; addr++){
		extHeap__.driver->write(addr, 0);
	}
}

// Returns the heap that corresponds to the given index (e.g intHeap if the index was 0)
Heap* os_lookupHeap(uint8_t index){
	switch(index){
		case 0: return intHeap;
		case 1: return extHeap;
		default: return 0;
	}
}

// Returns the number of Heaps.
size_t os_getHeapListLength(){
	return 2; // intHeap and extHeap
}