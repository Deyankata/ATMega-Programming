
/*
 * os_mem_drivers.h
 *
 * Contains management of several RAM devices of the OS. 
 * It contains everything that is associated with low level memory access.
 *
 * Created: 15.11.2023 22:33:19
 *  Author: bx541002
 */ 


#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#include "defines.h"

#include <inttypes.h>

#define INTERNAL_SRAM_SIZE 4096
#define INTERNAL_SRAM_START (0x100)
#define EXTERNAL_SRAM_SIZE ((uint16_t)65535)
#define EXTERNAL_SRAM_START (0x0)

// External SRAM instructions
#define READ_INSTRUCTION (0x03)
#define WRITE_INSTRUCTION (0x02)
#define WRMR_INSTRUCTION (0x01)

// Type used instead of uint8_t* pointers to avoid direct dereferencing
typedef uint16_t MemAddr;

// Type for a single value (used instead of uint8_t to increase readability)
typedef uint8_t MemValue;

// The data structure for a memory driver such as intSRAM
typedef struct MemDriver{
	// Constants for storage medium characteristics
	uint16_t sramSize;
	MemAddr sramStart;
	
	void (*init)(void);
	MemValue (*read)(MemAddr addr);
	void (*write)(MemAddr addr, MemValue value);
} MemDriver;

// This specific MemDriver is initialized in os_mem_drivers.c
extern MemDriver intSRAM__;
#define intSRAM (&intSRAM__)

// This specific MemDriver is initialized in os_mem_drivers.c
extern MemDriver extSRAM__;
#define extSRAM (&extSRAM__)

// Initialize all memory devices
void initMemoryDevices(void);

#endif /* OS_MEM_DRIVERS_H_ */