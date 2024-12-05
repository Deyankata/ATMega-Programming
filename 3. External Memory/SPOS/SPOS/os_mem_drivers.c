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
#include "os_scheduler.h"
#include "os_spi.h"
#include <avr/interrupt.h>
#include <avr/io.h>

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
	.sramSize = INTERNAL_SRAM_SIZE,
	.sramStart = INTERNAL_SRAM_START,
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal
};

void initSRAM_external(void){
	// Initialize the SPI-Module
	os_spi_init();
	
	// Send byte-by-byte access mode command
	PORTB &= ~(1 << DDB4); // CS-Pin to LOW (select slave)
	os_spi_send(WRMR_INSTRUCTION);
	os_spi_send(0x00);
	PORTB |= (1 << DDB4); // CS-Pin to HIGH (deactivate slave)
}

//Private function to read a single byte from the external SRAM
MemValue readSRAM_external(MemAddr addr){
	os_enterCriticalSection();
	
	MemValue data;
	
	PORTB &= ~(1 << DDB4); // CS-Pin to LOW (select slave)
	
	// Send READ instruction
	os_spi_send(READ_INSTRUCTION);
	os_spi_send(0x00);
	
	// Send the address (16-bit)
	os_spi_send((MemValue)(addr >> 8)); // Send the upper byte
	os_spi_send((MemValue)addr); // Send the lower byte
	
	data = os_spi_receive();
	
	PORTB |= (1 << DDB4); // CS-Pin to HIGH (deactivate slave)
	
	os_leaveCriticalSection();
	
	return data;
}

// Private function to write a single byte to the external SRAM
void writeSRAM_external(MemAddr addr, MemValue value){
	os_enterCriticalSection();
	
	PORTB &= ~(1 << DDB4); // CS-Pin to LOW (select slave)
	
	// Send WRITE instruction
	os_spi_send(WRITE_INSTRUCTION);
	os_spi_send(0x00);
	
	// Send the address (16-bit)
	os_spi_send((MemValue)(addr >> 8));
	os_spi_send((MemValue)addr);
	
	// Send the value byte to be written
	os_spi_send(value);
	
	PORTB |= (1 << DDB4); // // CS-Pin to HIGH (deactivate slave)
	
	os_leaveCriticalSection();
}

MemDriver extSRAM__ = {
	.sramSize = EXTERNAL_SRAM_SIZE,
	.sramStart = EXTERNAL_SRAM_START,
	.init = initSRAM_external,
	.read = readSRAM_external,
	.write = writeSRAM_external
};

