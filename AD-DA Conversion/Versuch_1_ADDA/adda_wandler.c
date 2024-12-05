#include "adda_wandler.h"
#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

// R-2R-Netzwerk (DA-Wandler)

void da_initInput(void){
	// configure DDRD as input
	DDRD &= 0b00000000;
	// activate Pullup-Resistor
	PORTD |= 0b11111111;
}

void da_initOutput(void){
	// configure DDRA as output
	DDRA |= 0b11111111;
	// output 1 at PORTA
	PORTA |= 0b11111111;

	// configure DDRB as output
	DDRB |= 0b11111111;
	// output 1 for all pins in PORTB
	PORTB |= 0b11111111;
}

void manuell(void){
	da_initInput();
	da_initOutput();	
	while(1){
		if (PIND != PORTA){
			PORTA = PIND;
			PORTB = PIND;
		}
	}
}



// Sukzessive-Approximation-Register (AD-Wandler)

void sar_track_initOutput(void){
	// Set Port B as output
	DDRB |= 0b11111111;
	PORTB |= 0b11111111;

	// Set Port A as output
	DDRA |= 0b11111111;
	PORTA |= 0b11111111;
}

// Converts an array of numbers to a binary number
uint8_t array_to_binary(int arr[], int length) {
	uint8_t result = 0;

	for (int i = 0; i < length; i++) {
		// Shift the current result one bit to the left and bitwise OR with the current array element
		result = (result << 1) | arr[i];
	}

	return result;
}

void sar(void){	
	
	uint8_t ref_sar = 0b00000000;
	os_initInput();
	while (1){
		sar_track_initOutput();		
		os_waitForInput();
		ref_sar = 0b10000000;
		int refArr[8] = {1, 0, 0, 0, 0, 0, 0, 0}; // Array, whose elements correspond to the ref_sar binary value
		for (int i = 1; i <= 8; i++){
			PORTB = ref_sar;
			PORTA = ~ref_sar;
			_delay_ms(50);
			if ((PINC & 0b00000001) == 1){ // if Umess < Uref
				refArr[i-1] = 0;
				refArr[i] = 1;
				ref_sar = array_to_binary(refArr, 8);
			}
			else {						   // if Umess > Uref
				refArr[i] = 1;
				ref_sar = array_to_binary(refArr, 8);
			}
			
		}
		os_waitForNoInput();		
	}
}




// Tracking-Wandler (AD-Wandler)

void tracking(void){
	
	uint8_t ref_track = 0b00000000;
	os_initInput();
	while(1){
		sar_track_initOutput();
		os_waitForInput();
		ref_track = 0b00000000; // reset after each iteration
		while(1){
			PORTB = ref_track;
			PORTA = ~ref_track;
			_delay_ms(50);
			if((PINC & 0b00000001) == 0){	// if Umess > Uref
				ref_track += 1;
			}
			else{							// if Umess < Uref
				ref_track -= 1;
				PORTB = ref_track;
				PORTA = ~ref_track;
				break;
			}
		}
		os_waitForNoInput();
	}
}