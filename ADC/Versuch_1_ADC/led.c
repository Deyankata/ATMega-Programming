#include "led.h"

#include <avr/io.h>

uint16_t activateLedMask = 0xFFFF;
// Split activateLedMask into two seperate 8-bit values
uint8_t lowByteLedMask;
uint8_t highByteLedMask;
/*!
 *  Initializes the led bar. Note: All PORTs will be set to output.
 */
void initLedBar(void) {
	lowByteLedMask = (uint8_t)activateLedMask;
	highByteLedMask = (uint8_t)(activateLedMask >> 8);

	DDRA |= lowByteLedMask;
	PORTA |= lowByteLedMask;
	
	DDRD |= highByteLedMask;
	PORTD |= highByteLedMask;
}

/*!
 *  Sets the passed value as states of the led bar (1 = on, 0 = off).
 */
void setLedBar(uint16_t value) {
	// Split value into two separate 8-bit values
	uint8_t lowByte = (uint8_t)value;
	uint8_t highByte = (uint8_t)(value >> 8);

	uint8_t lowByteLedMask = (uint8_t)activateLedMask;
	uint8_t highByteLedMask = (uint8_t)(activateLedMask >> 8);

	PORTA = ~(lowByte) & lowByteLedMask;
	PORTD = ~(highByte) & highByteLedMask;
}
