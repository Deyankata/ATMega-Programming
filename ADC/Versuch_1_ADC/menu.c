#include "menu.h"

#include "adc.h"
#include "bin_clock.h"
#include "lcd.h"
#include "led.h"
#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

/*!
 *  Hello world program.
 *  Shows the string 'Hello World!' on the display.
 */
void helloWorld(void) {
	// Repeat until ESC gets pressed
	while(1){
		lcd_writeProgString(PSTR("Hallo Welt!"));
		_delay_ms(500);
		lcd_clear();
		_delay_ms(500);
		
		if (os_getInput() == 0b00001000){
			break;
		}
	}
}

/*!
 *  Shows the clock on the display and a binary clock on the led bar.
 */
void displayClock(void) {
	lcd_clear();
	uint16_t clockVal;

	// Repeat until ESC gets pressed
	while(1){
		uint8_t hh = getTimeHours();
		uint8_t mm = getTimeMinutes();
		uint8_t ss = getTimeSeconds();
		uint16_t mmm = getTimeMilliseconds();
		
		clockVal = ((uint16_t)hh << 12) | ((uint16_t)mm << 6) | ((uint16_t)ss);
		setLedBar(clockVal);

		char displayTime[13];
		snprintf(displayTime, sizeof(displayTime), "%02d:", hh);
		snprintf(displayTime + 3, sizeof(displayTime) - 3, "%02d:", mm);
		snprintf(displayTime + 6, sizeof(displayTime) - 6, "%02d:", ss);
		snprintf(displayTime + 9, sizeof(displayTime) - 9, "%03d", mmm);
		
		lcd_line2();
		lcd_writeString(displayTime);
		
		if (os_getInput() == 0b00001000){
			lcd_clear();
			setLedBar(0x0000);
			break;
		}
	}
}

/*!
 *  Shows the stored voltage values in the second line of the display.
 */
void displayVoltageBuffer(uint8_t displayIndex) {
	uint16_t voltageToDisplay = getStoredVoltage(displayIndex);
	char displayIndexText[11];
	snprintf(displayIndexText, sizeof(displayIndexText), "%03d/100: ", displayIndex);
	
	lcd_line2();
	lcd_writeString(displayIndexText);
	lcd_writeVoltage(voltageToDisplay, 1023, 5);
}

/*!
 *  Shows the ADC value on the display and on the led bar.
 */
void displayAdc(void) {
	lcd_clear();
	uint16_t adcResult;
	uint8_t bufferIndex = 0;
	// Repeat until ESC gets pressed
	while(1){
		uint16_t ledValue = 0;
		// LCD-Display
		adcResult = getAdcValue();
		lcd_line1();
		lcd_writeVoltage(adcResult, 1023, 5);
			
		// LED-Bargraph
		while (adcResult >= 68){
			ledValue = ledValue << 1;
			ledValue |= 2;
			adcResult -= 68;
		}
		setLedBar(ledValue);
		
		_delay_ms(100);
		
		// Store measured values
		if (os_getInput() & 0b00000001){ // Press Enter
			storeVoltage();
			_delay_ms(100);
		}
		if((os_getInput() & 0b00000100) >> 2){ // UP
			if (bufferIndex == 100){
				bufferIndex = 0;
			}
			bufferIndex += 1;
			displayVoltageBuffer(bufferIndex);
			_delay_ms(100);
		}
		else if((os_getInput() & 0b00000010) >> 1){ // DOWN
			if (bufferIndex == 0){
				bufferIndex = 100;
			}
			else{
				bufferIndex--;
			}
			displayVoltageBuffer(bufferIndex);
			_delay_ms(100);
		}
		
		if (os_getInput() == 0b00001000){ // Esc
			lcd_clear();
			setLedBar(0x0000);
			break;
		}
	}
}

/*! \brief Starts the passed program
 *
 * \param programIndex Index of the program to start.
 */
void start(uint8_t programIndex) {
    // Initialize and start the passed 'program'
    switch (programIndex) {
        case 0:
            lcd_clear();
            helloWorld();
            break;
        case 1:
            activateLedMask = 0xFFFF; // Use all LEDs
            initLedBar();
            initClock();
            displayClock();
            break;
        case 2:
            activateLedMask = 0xFFFE; // Don't use LED 0
            initLedBar();
            initAdc();
            displayAdc();
            break;
        default:
            break;
    }

    // Do not resume to the menu until all buttons are released
    os_waitForNoInput();
}

/*!
 *  Shows a user menu on the display which allows to start subprograms.
 */
void showMenu(void) {
    uint8_t pageIndex = 0;

    while (1) {
        lcd_clear();
        lcd_writeProgString(PSTR("Select:"));
        lcd_line2();

        switch (pageIndex) {
            case 0:
                lcd_writeProgString(PSTR("1: Hello world"));
                break;
            case 1:
                lcd_writeProgString(PSTR("2: Binary clock"));
                break;
            case 2:
                lcd_writeProgString(PSTR("3: Internal ADC"));
                break;
            default:
                lcd_writeProgString(PSTR("----------------"));
                break;
        }

        os_waitForInput();
        if (os_getInput() == 0b00000001) { // Enter
            os_waitForNoInput();
            start(pageIndex);
        } else if (os_getInput() == 0b00000100) { // Up
            os_waitForNoInput();
            pageIndex = (pageIndex + 1) % 3;
        } else if (os_getInput() == 0b00000010) { // Down
            os_waitForNoInput();
            if (pageIndex == 0) {
                pageIndex = 2;
            } else {
                pageIndex--;
            }
        }
    }
}
