/*
 * joystick.h
 *
 * Functions to access joystick.
 *
 * Created: 13.01.2024 12:31:50
 *  Author: bx541002
 */ 


#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdbool.h>
#include <stdint.h>

/*
Calculate the tolerance for the neutral position:

Tolerance in ADC units = (Tolerance in volts / Reference voltage) * ADC Resolution
=> Tolerance in ADC units = (1V / 5V) * 1024 = 204
*/
#define TOLERANCE 204 

// Possible joystick directions
typedef enum Direction {
	JS_LEFT,
	JS_RIGHT,
	JS_UP,
	JS_DOWN,
	JS_NEUTRAL
} Direction;

// Initialize joystick registers
void js_init(void);

// Calculates a Direction from current joystick position
Direction js_getDirection(void);

uint16_t js_getAnalogValue(uint8_t channel);

// Read vertical joystick displacement
uint16_t js_getVertical(void);

// Read horizontal joystick displacement
uint16_t js_getHorizontal(void);

// Getter for the joystick button state
bool js_getButton(void);

// Read joystick displacement
uint16_t os_getJoystick(uint8_t pin);

// Blocks until joystick button is pressed
void os_waitForJoystickButtonInput(void);

// Blocks until joystick button is released
void os_waitForNoJoystickButtonInput(void);


#endif /* JOYSTICK_H_ */