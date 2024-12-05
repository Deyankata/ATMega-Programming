/*! \file
 *  \brief Functions to draw premade things on the LED Panel
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2019
 */
#include "led_paneldriver.h"

#include "defines.h"
#include "util.h"

#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>

// Define global variables for frame buffer, current double row, and current level
uint8_t currentDoubleRow = 0;
uint8_t currentLevel = 0;
uint8_t frameBuffer[NUM_LEVELS][NUM_DOUBLE_ROWS][32];

//! \brief Enable compare match interrupts for Timer 1
void panel_startTimer() {
    sbi(TIMSK1, OCIE1A);
}

//! \brief Disable compare match interrupts for Timer 1
void panel_stopTimer() {
    cbi(TIMSK1, OCIE1A);
}

//! \brief Initialization function of Timer 1
void panel_initTimer() {
    // Configuration TCCR1B register
    sbi(TCCR1B, WGM12); // Clear on timer compare match
    sbi(TCCR1B, CS12);  // Prescaler 256  1
    cbi(TCCR1B, CS11);  // Prescaler 256  0
    cbi(TCCR1B, CS10);  // Prescaler 256  0

    // Output Compare register 256*7 = 1792 tics => interrupt interval approx 0.0896 ms
    OCR1A = 0x0007;
}

//! \brief Initializes used ports of panel
void panel_init(){
	// Configure row selection pins as outputs
	DDRA |= (1 << DDA0) | (1 << DDA1) | (1 << DDA2) | (1 << DDA3);
		
	// Configure PORTC pins as outputs
	DDRC |= (1 << PIN_C_CLK) | (1 << PIN_C_LE) | (1 << PIN_C_OE);
	PORTC |= (1 << PIN_C_OE); // Disable /OE
	PORTC &= ~(1 << PIN_C_CLK); // Set CLK to 0
	PORTC &= ~(1 << PIN_C_LE); // Set LE to 0
	
	// Configure PORTD pins as outputs
	DDRD |= (1 << PIN_D_R1) | (1 << PIN_D_G1) | (1 << PIN_D_B1) |
			(1 << PIN_D_R2) | (1 << PIN_D_G2) | (1 << PIN_D_B2);
}

// Function to enable latch
void panel_latchEnable(){
	PORTC |= (1 << PIN_C_LE);
}

// Function to disable latch
void panel_latchDisable(){
	PORTC &= ~(1 << PIN_C_LE);
}

// Function to enable output (OE)
void panel_outputEnable(){
	PORTC &= ~(1 << PIN_C_OE); // Inverted logic, 0 is enabled
}

// Function to disable output (OE)
void panel_outputDisable(){
	PORTC |= (1 << PIN_C_OE); // Inverted logic, 1 is disabled
}

// Function to set the address of the double row
void panel_setAdress(uint8_t doubleRow){
	// Set the 4-bit address on the decoder pins
	PORTA = (PORTA & 0xF0) | (doubleRow & 0x0F);
}

void panel_setOutput(uint8_t colorData){
	PORTD = (0b00111111 & colorData);
}

// Function to clock the shift registers
void panel_CLK(){
	PORTC |= (1 << PIN_C_CLK);
	PORTC &= ~(1 << PIN_C_CLK);
}

//! \brief ISR to refresh LED panel, trigger 1 compare match interrupts
ISR(TIMER1_COMPA_vect) {
	// Iterate over 32 runs to update the current double row
	for (uint8_t i = 0; i < 32; i++){
		// Set the output values based on the frame buffer and current level
		panel_setOutput(frameBuffer[currentLevel][currentDoubleRow][i]);
		
		// Clock the shift register (rising edge)
		panel_CLK();
	}
	panel_outputDisable();
	
	// Set the address of the current double row
	panel_setAdress(currentDoubleRow);
	
	// Enable latch, disable latch, and then enable output
	panel_latchEnable();
	panel_latchDisable();
	
	panel_outputEnable();

	// Increment the double row for the next iteration
	currentDoubleRow++;
	
	// After 16 runs, switch to the next level
	if (currentDoubleRow == 16){
		currentDoubleRow = 0;
		// Reset currentLevel if it reaches max level
		if (++currentLevel == NUM_LEVELS){
			currentLevel = 0;
		}
	}
}
