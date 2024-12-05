/*
 * os_spi.c
 *
 * Basic functions for setup and use of the SPI module.
 *
 * Created: 29.11.2023 15:57:46
 *  Author: bx541002
 */

#include "os_spi.h"

// Wait for the SPI transmission to complete
void wait_for_spi(){
	while (!(SPSR & (1 << SPIF))); // Wait for the SPIF-bit
}

void os_spi_init(){
	// Configure the relevant I/O pins (external SRAM in Slave mode)
	DDRB |= (1 << DDB7) | (1 << DDB5) | (1 << DDB4); // CLK, MOSI, \CS as output
	PORTB |= (1 << PB7) | (1 << PB5) | (1 << PB4);
	
	DDRB &= ~(1 << DDB6); // MISO as input
	PORTB |= (1 << PB6);
	
	// Activate the microcontroller as master of the SPI bus
	SPCR |= (1 << MSTR);
	
	// Select the SPI mode compatible with the 23LC1024 (Idle Low, Active High, Leading Edge)
	SPCR &= ~(1 << CPOL); // Set Clock Polarity (CPOL) to 0
	SPCR &= ~(1 << CPHA); // Set Clock Phase (CPHA) to 0
	
	// Set the clock frequency for communication (Here: SPI-Clock = F_CPU / 2)
	SPCR &= ~(1 << SPR0);
	SPCR &= ~(1 << SPR1);
	SPSR |= (1 << SPI2X);
	
	// Activate the SPI
	SPCR |= (1 << SPE);
}

uint8_t os_spi_send(uint8_t data){
	// Send data
	SPDR = data;
	
	// Wait for the current transmission to complete
	wait_for_spi();
	
	// Return a clever value
	return SPDR;
}

uint8_t os_spi_receive(){
	return os_spi_send(0xFF);
}