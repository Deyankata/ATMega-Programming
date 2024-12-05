/*
 * joystick.c
 *
 * Functions to access joystick.
 *
 * Created: 13.01.2024 12:31:26
 *  Author: bx541002
 */ 

#include "joystick.h"
#include <avr/io.h>

void js_init(){
	// Configure ADMUX register for internal reference voltage and initial pin selection
	ADMUX = (1 << REFS0);
	
	// Enable ADC, set prescaler to 128 for better resolution
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// Configure input pins as input
	DDRA &= ~((1 << PA5) | (1 << PA6) | (1 << PA7));
	
	// Configure button pin as input
	DDRA &= ~(1 << DDA7);
	PORTA |= (1 << PA7);
}

uint16_t js_getAnalogValue(uint8_t channel){
	// Configure MUX bits for the desired analog channel
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	
	// Start conversion
	ADCSRA |= (1 << ADSC);
	
	// Wait for conversion to complete
	while (ADCSRA & (1 << ADSC));
	
	// Return result
	return ADC;
}

uint16_t js_getHorizontal(){
	return js_getAnalogValue(5); // A5 for horizontal axis
}

uint16_t js_getVertical(){
	return js_getAnalogValue(6); // A6 for vertical axis
}

Direction js_getDirection(){
	uint16_t horizontal = js_getHorizontal();
	uint16_t vertical = js_getVertical();
	
	if (horizontal > 512 + TOLERANCE){
		return JS_RIGHT;
	}
	else if (horizontal < 512 - TOLERANCE){
		return JS_LEFT;
	}
	else if (vertical > 512 + TOLERANCE){
		return JS_UP;
	}
	else if (vertical < 512 - TOLERANCE){
		return JS_DOWN;
	}
	else {
		return JS_NEUTRAL;
	}
}

bool js_getButton(){
	return ((~PINA & 0b10000000) >> 7) == 1; // Check if button is pressed (active low)
}

void os_waitForJoystickButtonInput(){
	while (!js_getButton()){
		// Wait for button to be pressed
	}
}

void os_waitForNoJoystickButtonInput(){
	while (js_getButton()){
		// Wait for button to be released
	}
}