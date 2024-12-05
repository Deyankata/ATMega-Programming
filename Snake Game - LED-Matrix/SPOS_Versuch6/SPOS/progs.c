//-------------------------------------------------
//          TestTask: Snake
//-------------------------------------------------

#include "defines.h"
#include "joystick.h"
#include "lcd.h"
#include "led_draw.h"
#include "led_paneldriver.h"
#include "led_patterns.h"
#include "os_core.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"
#include "led_snake.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>
#include <util/atomic.h>

#if VERSUCH != 2
#warning "Please fix the VERSUCH-define"
#endif

REGISTER_AUTOSTART(snake)
void snake(void){
	// Initialize LED matrix and timer
	panel_init();
	panel_initTimer();
	panel_startTimer();
	js_init();
	
	runGame();
}