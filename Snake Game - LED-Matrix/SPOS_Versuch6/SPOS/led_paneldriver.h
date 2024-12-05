/*! \file
 *  \brief Low level functions to draw premade things on the LED Panel
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2019
 */
#ifndef _LED_PANELDRIVER_H
#define _LED_PANELDRIVER_H
#include <avr/io.h>
#include <stdbool.h>

// Define constants pin configurations
#define PIN_C_CLK    PC0
#define PIN_C_LE     PC1
#define PIN_C_OE     PC6

#define PIN_D_R1     PD0
#define PIN_D_G1     PD1
#define PIN_D_B1     PD2
#define PIN_D_R2     PD3
#define PIN_D_G2     PD4
#define PIN_D_B2     PD5

#define NUM_DOUBLE_ROWS 16
#define NUM_LEVELS 3

#define LEVEL_1 0
#define LEVEL_2 1
#define LEVEL_3 2

// Define global variable for frame buffer
extern uint8_t frameBuffer[NUM_LEVELS][NUM_DOUBLE_ROWS][32];

//! Initializes registers
void panel_init();

//! Starts interrupts
void panel_startTimer(void);

//! Stops interrupts
void panel_stopTimer(void);

//! Initializes interrupt timer
void panel_initTimer(void);

void panel_latchEnable(void);
void panel_latchDisable(void);

void panel_outputEnable(void);
void panel_outputDisable(void);

void panel_setAdress(uint8_t doubleRow);

void panel_setOutput(uint8_t colorData);

void panel_CLK(void);

#endif
