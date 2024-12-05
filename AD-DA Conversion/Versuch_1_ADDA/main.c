#include "adda_wandler.h"

#define FUNCTION_INDEX 3		// Choose which function to run (1 - manuell(); 2 - sar(); 3 - tracking())

int main(void) {
	
	switch(FUNCTION_INDEX){
		case 1:
			manuell();
			break;
		case 2:
			sar();
			break;
		case 3:
			tracking();
			break;
		default:
			break;		
	}
}
