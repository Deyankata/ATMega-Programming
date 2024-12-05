/*
 * os_mem_drivers.c
 *
 * Memory Driver structs and their functions.
 *
 * Created: 15.11.2023 22:37:06
 *  Author: bx541002
 */

#include "os_mem_drivers.h"
#include "defines.h"

/* Pseudo-function to initialize the internal SRAM Actually, 
 * there is nothing to be done when initializing the internal SRAM,
 * but we create this function, because MemDriver expects one for every memory device.
 */
void initSRAM_internal(void){}
	
// Private function to read a value from the internal SRAM
MemValue readSRAM_internal(MemAddr addr){
	return *((MemValue *)addr);
}

// Private function to write a value to the internal SRAM
void writeSRAM_internal(MemAddr addr, MemValue value){
	*((MemValue *)addr) = value;
}

// Initialize intSRAM
MemDriver intSRAM__ = {
	.sramSize = SRAM_SIZE,
	.sramStart = SRAM_START,
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal
};