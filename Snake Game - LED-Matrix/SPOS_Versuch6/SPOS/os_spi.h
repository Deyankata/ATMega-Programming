/*
 * os_spi.h
 *
 * Basic functions for setup and use of the SPI module.
 *
 * Created: 29.11.2023 15:58:01
 *  Author: bx541002
 */

#include "util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <util/atomic.h> 

#ifndef OS_SPI_H_
#define OS_SPI_H_

// Configures relevant I/O registers/pins and initializes the SPI module.
void os_spi_init(void);

/*
Performs a SPI send. This method blocks until the data exchange is completed.
Additionally, this method returns the byte which is
received in exchange during the communication.
*/
uint8_t os_spi_send(uint8_t data);

// Performs a SPI read. This method blocks until the exchange is completed.
uint8_t os_spi_receive();

#endif /* OS_SPI_H_ */