/*! \file
 *  \brief High level functions to draw premade things on the LED Panel
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2019
 */
#include "led_draw.h"

#include "led_paneldriver.h"
#include "led_patterns.h"
#include "util.h"

//! \brief Distributes bits of given color's channels r, g and b on layers of framebuffer
void draw_setPixel(uint8_t x, uint8_t y, Color color) {
	uint8_t red = color.r;
	uint8_t green = color.g;
	uint8_t blue = color.b;
	
	if (y < 16){
		frameBuffer[LEVEL_1][y][x] = (frameBuffer[LEVEL_1][y][x] & 0b11111000) | ((red & 0x80) >> 7) | ((green & 0x80) >> 6) | ((blue & 0x80) >> 5);
		frameBuffer[LEVEL_2][y][x] = (frameBuffer[LEVEL_2][y][x] & 0b11111000) | ((red & 0x40) >> 6) | ((green & 0x40) >> 5) | ((blue & 0x40) >> 4);
		frameBuffer[LEVEL_3][y][x] = (frameBuffer[LEVEL_3][y][x] & 0b11111000) | ((red & 0x20) >> 5) | ((green & 0x20) >> 4) | ((blue & 0x20) >> 3);
	}
	else{
		frameBuffer[LEVEL_1][y-16][x] = (frameBuffer[LEVEL_1][y-16][x] & 0b11000111) | ((red & 0x80) >> 4) | ((green & 0x80) >> 3) | ((blue & 0x80) >> 2);
		frameBuffer[LEVEL_2][y-16][x] = (frameBuffer[LEVEL_2][y-16][x] & 0b11000111) | ((red & 0x40) >> 3) | ((green & 0x40) >> 2) | ((blue & 0x40) >> 1);
		frameBuffer[LEVEL_3][y-16][x] = (frameBuffer[LEVEL_3][y-16][x] & 0b11000111) | ((red & 0x20) >> 2) | ((green & 0x20) >> 1) | (blue & 0x20);
	}
}

//! \brief Reconstructs RGB-Color from layers of framebuffer
Color draw_getPixel(uint8_t x, uint8_t y) {
	uint8_t bufferColorFisrtLevel = (y < 16) ? frameBuffer[LEVEL_1][y][x] : frameBuffer[LEVEL_1][y-16][x];
	uint8_t bufferColorSecondLevel = (y < 16) ? frameBuffer[LEVEL_2][y][x] : frameBuffer[LEVEL_2][y-16][x];
	uint8_t bufferColorThirdLevel = (y < 16) ? frameBuffer[LEVEL_3][y][x] : frameBuffer[LEVEL_3][y-16][x];
	Color reconstructedColor = {};
	
	if (y < 16){
		reconstructedColor.r = ((bufferColorFisrtLevel & 0b00000001) << 7) | ((bufferColorSecondLevel & 0b00000001) << 6) | ((bufferColorThirdLevel & 0b00000001) << 5);
		reconstructedColor.g = ((bufferColorFisrtLevel & 0b00000010) << 6) | ((bufferColorSecondLevel & 0b00000010) << 5) | ((bufferColorThirdLevel & 0b00000010) << 4); 
		reconstructedColor.b = ((bufferColorFisrtLevel & 0b00000100) << 5) | ((bufferColorSecondLevel & 0b00000100) << 4) | ((bufferColorThirdLevel & 0b00000100) << 3);
	}
	else{
		reconstructedColor.r = ((bufferColorFisrtLevel & 0b00001000) << 4) | ((bufferColorSecondLevel & 0b00001000) << 3) | ((bufferColorThirdLevel & 0b00001000) << 2);
		reconstructedColor.g = ((bufferColorFisrtLevel & 0b00010000) << 3) | ((bufferColorSecondLevel & 0b00010000) << 2) | ((bufferColorThirdLevel & 0b00010000) << 1);
		reconstructedColor.b = ((bufferColorFisrtLevel & 0b00100000) << 2) | ((bufferColorSecondLevel & 0b00100000) << 1) | (bufferColorThirdLevel & 0b00100000);
	}
	
	return reconstructedColor;
}

//! \brief Fills whole panel with given color
void draw_fillPanel(Color color) {
	// Iterate through each pixel and set it with the given color
	for (uint8_t col = 0; col < 32; col++){
		for (uint8_t row = 0; row < 32; row++){
			draw_setPixel(col, row, color);
		}
	}
}

//! \brief Sets every pixel's color to black
void draw_clearDisplay() {
	// Iterate through each pixel and color it black
	for (uint8_t col = 0; col < 32; col++){
		for (uint8_t row = 0; row < 32; row++){
			draw_setPixel(col, row, COLOR_BLACK);
		}
	}
}

//! \brief Draws Rectangle
void draw_filledRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, Color color) {
	// Iterate through the rectangle's coordinates and set the pixels
	for (uint8_t y = y1; y <= y2; y++){
		for (uint8_t x = x1; x <= x2; x++){
			draw_setPixel(x, y, color);
		}
	}
}


/*! \brief Draws pattern
 * \param x			Column of left upper corner
 * \param y			Row of left upper corner
 * \param height	Height to be used for the pattern
 * \param width		Width to be used for the pattern
 * \param pattern	The given pattern. Has a maximum span of 8x8 pixel
 * \param color		RGB color used to draw the pattern with
 * \param overwrite Delete pixels in picture that are black in the pattern if set to true
 */
void draw_pattern(uint8_t x, uint8_t y, uint8_t height, uint8_t width, uint64_t pattern, Color color, bool overwrite) {
    uint64_t temprow = 0;
    for (uint8_t i = 0; i < height; i++) {
        // Select row
        temprow = pattern >> i * 8;

        for (uint8_t j = 0; j < width; j++) {
            if ((x + j < 32) && (y + i < 32)) {
                if (temprow & (1 << (7 - j))) {
                    draw_setPixel(x + j, y + i, color);
                } else {
                    if (overwrite) {
                        draw_setPixel(x + j, y + i, (Color){.r = 0, .g = 0, .b = 0});
                    }
                }
            }
        }
    }
}

/*! \brief Draws a character on the panel.
 * \param letter    A character of [0-9A-Za-z]. Note: small letters cannot be drawn, the corresponding capital letter will be drawn instead.
 * \param x         Column of left upper corner
 * \param y         Row of left upper corner
 * \param color     RGB color to draw letter with
 * \param overwrite Delete pixels in picture that are black in the pattern if set to true
 */
void draw_letter(char letter, uint8_t x, uint8_t y, Color color, bool overwrite, bool large) {
    const uint64_t *pattern_table;
    uint8_t idx;

    // the type of character determines how we get our lookup table and index
    if (letter >= '0' && letter <= '9') {
        pattern_table = large ? led_large_numbers : led_small_numbers;
        idx = letter - '0';
    } else if (letter >= 'A' && letter <= 'Z') {
        pattern_table = large ? led_large_letters : led_small_letters;
        idx = letter - 'A';
    }

    else if (letter >= 'a' && letter <= 'z') {
        pattern_table = large ? led_large_letters : led_small_letters;
        idx = letter - 'a';
    } else {
        return;
    }

    // in all cases, we have to fetch our pattern from a progmem array of patterns
    // prepare a SRAM uint64_t value to store the pattern
    uint64_t pattern;

    // Since there is no load function for uint64_ts, we will just use the Flash-to-SRAM version of memcpy
    memcpy_P(&pattern, pattern_table + idx, sizeof(uint64_t));
    draw_pattern(x, y, large ? LED_CHAR_HEIGHT_LARGE : LED_CHAR_HEIGHT_SMALL, large ? LED_CHAR_WIDTH_LARGE : LED_CHAR_WIDTH_SMALL, pattern, color, overwrite);
}


/*! \brief Draws Decimal (0..9) on panel
 * \param dec       Decimal number (from 0 to 9)
 * \param x         Row of left upper corner
 * \param y         Column of left upper corner
 * \param color     Color to draw number with
 * \param overwrite Delete pixels in picture that are black in the pattern if set to true
 * \param large     Draws large numbers when set to true, otherwise small (small: 5x3 px, large: 7x5 px)
 */
void draw_decimal(uint8_t dec, uint8_t x, uint8_t y, Color color, bool overwrite, bool large) {
    draw_letter(dec + '0', x, y, color, overwrite, large);
}

/*! \brief Draw an integer on the panel
 * \param number The number to draw
 * \param right_align if true, the least-significant digit of the number will be drawn at x,y; otherwise, the number will start at this position
 * \param x		   Column of the top-left corner of either the first or the last digit, depending on right_align
 * \param y		   Row of the top-left corner of either the first or the large digit, depending on right_align
 * \param overwrite Delete pixels in picture that are black in the pattern if set to true
 * \param large     Draws large numbers when set to true, otherwise small (small: 5x3 px, large: 7x5 px)
 */
void draw_number(uint32_t number, bool right_align, uint8_t x, uint8_t y, Color color, bool overwrite, bool large) {
    char number_chars[10];
    uint8_t len = 0;

    uint32_t temp = number;

    while (temp >= 10) {
        number_chars[len++] = '0' + (temp % 10);
        temp /= 10;
    }
    number_chars[len++] = '0' + temp;

    const uint8_t diff = (large ? LED_CHAR_WIDTH_LARGE : LED_CHAR_WIDTH_SMALL) + 1;

    // this could potentially be <0 but unsigned integer underflow is defined
    // so that should not be a problem
    // we will simply land somewhere below 256 which will be safely cut off
    // by drawPattern, so the number is just truncated at the left
    if (right_align) x -= diff * (len - 1);
    for (uint8_t i = 0; i < len; i++) {
        draw_letter(number_chars[len - 1 - i], x + i * diff, y, color, overwrite, large);
    }
}
