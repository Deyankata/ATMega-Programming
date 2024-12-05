/*
 * led_snake.c
 *
 * Created: 14.01.2024 12:51:22
 *  Author: bx541002
 */ 

#include "led_snake.h"
#include "led_draw.h"
#include "led_paneldriver.h"
#include "lcd.h"

#include <stdlib.h>
#include <util/delay.h>
#include <time.h>

// Snake variables
uint8_t snakeHeadX;
uint8_t snakeHeadY;
uint8_t snakeHeadDirection;
uint16_t snakeLength;

RingBuffer snakeBuffer;

// Game state variables
uint8_t highScore = 0;
uint8_t currentScore = 0;
uint8_t foodX, foodY; // Position of the food
uint8_t foodEaten = 0;

void initSnake(uint8_t startX, uint8_t startY){
	// Initialize snake
	snakeHeadX = startX;
	snakeHeadY = startY;
	snakeHeadDirection = RIGHT;
	snakeLength = 1;
	
	// Initialize ring buffer for snake
	snakeBuffer.head = 0;
	snakeBuffer.tail = 0;
}

void initGameState(){
	draw_clearDisplay();
	
	// Draw a line to separate the score and the game field
	for (uint8_t i = 0; i < 32; i++){
		draw_setPixel(i, SCORE_LINE_ROW, COLOR_BLUE);
	}
	
	// Draw the score
	currentScore = 0;
	draw_number(currentScore, true, 10, 0, COLOR_WHITE, false, false);
	draw_number(highScore, true, 20, 0, COLOR_WHITE, false, false);
	
	// Initialize the snake at the center of the board
	initSnake(NUM_COLUMNS / 2, NUM_ROWS / 2); 
	
	// Draw the snake head
	draw_setPixel(snakeHeadX, snakeHeadY, COLOR_GREEN);
	
	// Generate initial food Position
	generateFood(); 
}

void updateGameState(){
	Direction joystickDirection = js_getDirection();
	updateSnakeDirection(joystickDirection);

	updateRingBuffer();
	
	// Move the snake;
	moveSnake();

	// Check for collisions
	if (checkCollision()){
		// Game over
		if (currentScore > highScore){
			highScore = currentScore; // Update high score if necessary
		}
		gameOver(); // Reset the game for a new game
	}

	// Check if the snake has eaten the food
	if (checkFoodEaten(foodX, foodY)){
		currentScore++;
		generateFood(); // Generate new food position
	}
	
	// Draw Score
	draw_filledRectangle(10, 0, 22, 4, COLOR_BLACK);
	draw_number(currentScore, true, 10, 0, COLOR_WHITE, false, false);
	draw_number(highScore, true, 20, 0, COLOR_WHITE, false, false);
}

void moveSnake(){ 
	// Delete pixel at old position
	draw_setPixel(snakeHeadX, snakeHeadY, COLOR_BLACK);
	
	// Move the head based on the current direction
	switch (snakeHeadDirection){
		case RIGHT: // Right
			snakeHeadX++;
			break;
		case LEFT: // Left
			snakeHeadX--;
			break;
		case UP: // Up
			snakeHeadY--;
			break;
		case DOWN: // Down
			snakeHeadY++;
			break;
	}
	
	// Draw the snake
	drawSnake();
}

void updateSnakeDirection(Direction newDirection){
	switch (newDirection){
		case JS_RIGHT:
			snakeHeadDirection = RIGHT;
			break;
		case JS_LEFT:
			snakeHeadDirection = LEFT;
			break;
		case JS_UP:
			snakeHeadDirection = UP;
			break;
		case JS_DOWN:
			snakeHeadDirection = DOWN;
			break;
		default:
			break;
	}
}

void updateRingBuffer(){
	if (snakeLength > 1){
		snakeBuffer.head = (snakeBuffer.head + 2) % RINGBUFFER_SIZE;
		uint16_t headByte = snakeBuffer.head / 8; // Get the byte number of the buffer to which head is pointing
		uint8_t headBit = snakeBuffer.head % 8; // Get the specific bit position in that byte
		
		// Update the buffer, based on the current bit head is pointing to
		switch (headBit){
			case 0:
				snakeBuffer.directions[headByte] = (snakeBuffer.directions[headByte] & 0b11111100) | snakeHeadDirection;
				break;
			case 2:
				snakeBuffer.directions[headByte] = (snakeBuffer.directions[headByte] & 0b11110011) | (snakeHeadDirection << 2);
				break;
			case 4:
				snakeBuffer.directions[headByte] = (snakeBuffer.directions[headByte] & 0b11001111) | (snakeHeadDirection << 4);
				break;
			case 6:
				snakeBuffer.directions[headByte] = (snakeBuffer.directions[headByte] & 0b00111111) | (snakeHeadDirection << 6);
				break;
		}
		
		// If eaten, reset food flag
		if (foodEaten == 1){
			foodEaten = 0;
		}
		// If the food is not eaten increment the tail pointer, otherwise don't increment, since this effectively increases the length of the snake
		else{
			snakeBuffer.tail = (snakeBuffer.tail + 2) % RINGBUFFER_SIZE;
		}
	}	
}

uint8_t getBufferBitPair(uint16_t bitPairPosition){
	// Calculate array index and the specific bit index
	uint16_t arrayByte = bitPairPosition / 8;
	uint8_t bitPosition = bitPairPosition % 8;
	
	// Use a bitPairMask to return the desired bit-pair
	switch (bitPosition){
		case 0:
			return (snakeBuffer.directions[arrayByte] & 0b00000011);
		case 2: 
			return ((snakeBuffer.directions[arrayByte] & 0b00001100) >> 2);
		case 4:
			return ((snakeBuffer.directions[arrayByte] & 0b00110000) >> 4);
		case 6:
			return ((snakeBuffer.directions[arrayByte] & 0b11000000) >> 6);
	}
	
	return 0;
}

void drawSnake(){
	// If there is only the head of the snake, draw just the head
	if (snakeLength == 1){
		draw_setPixel(snakeHeadX, snakeHeadY, COLOR_YELLOW);
		return;
	}
	
	// Draw the head of the snake
	draw_setPixel(snakeHeadX, snakeHeadY, COLOR_YELLOW);
	
	// Initialize auxiliary variables
	uint8_t bitPair;
	uint8_t bodyX = snakeHeadX;
	uint8_t bodyY = snakeHeadY;
	uint16_t head = snakeBuffer.head;
	uint16_t tail = snakeBuffer.tail;
	
	// Calculate the coordinates of the first element from the buffer and draw it
	bitPair = getBufferBitPair(head);
	switch (bitPair){
		case RIGHT:
			bodyX--;
			break;
		case LEFT:
			bodyX++;
			break;
		case UP:
			bodyY++;
			break;
		case DOWN:
			bodyY--;
			break;
	}
	draw_setPixel(bodyX, bodyY, COLOR_GREEN);
	
	// Calculate the coordinates of the tail, tracing the path from the head to the tail
	while (head != tail){
		head = (head - 2) % RINGBUFFER_SIZE;
		
		bitPair = getBufferBitPair(head);	
		switch (bitPair){
			 case RIGHT:
				bodyX--;
				break;
			case LEFT:
				bodyX++;
				break;
			case UP:
				bodyY++;
				break;
			case DOWN:
				bodyY--;
				break;
		}
	}
	
	// Remove the last element that the tail points to
	draw_setPixel(bodyX, bodyY, COLOR_BLACK);
}

bool checkCollision(){
	 // Check for collision with walls
	 if (snakeHeadX >= NUM_COLUMNS || snakeHeadX < 0 || snakeHeadY >= NUM_ROWS || snakeHeadY <= SCORE_LINE_ROW){
		 return true;
	 }
	 
	 // Check for collision with itself
	 uint8_t bitPair;
	 uint16_t head = snakeBuffer.head;
	 uint16_t tail = snakeBuffer.tail;
	 
	 uint8_t collisionX = snakeHeadX;
	 uint8_t collisionY = snakeHeadY;
	 
	 // Trace back through the buffer to calculate the coordinates of each snake bit
	 while (head != tail){
		 bitPair = getBufferBitPair(head);
		 switch (bitPair){
			 case RIGHT:
				collisionX--;
				break;
			case LEFT:
				collisionX++;
				break;
			case UP:
				collisionY++;
				break;
			case DOWN:
				collisionY--;
				break;
		 }
		 
		 // Check for collision
		 if (snakeHeadX == collisionX && snakeHeadY == collisionY){
			 return true;
		 }
		 head = (head - 2) % RINGBUFFER_SIZE;
	 }
	 
	 return false; // No collision
}

bool checkFoodEaten(uint8_t foodX, uint8_t foodY){
	// Check if the head of the snake is at the position of the food
	if (snakeHeadX == foodX && snakeHeadY == foodY){
		// Remove the food, by setting it to the color of the snake
		draw_setPixel(foodX, foodY, COLOR_GREEN);
		
		// Increase the length of the snake
		if (snakeLength < MAX_SNAKE_LENGTH){
			snakeLength++;
		}
		
		foodEaten = 1;
		
		return true; // Food eaten
	}
	
	return false; // No food eaten
}

void generateFood(){
	// Use rand() to generate random positions for the food	
	foodX = rand() % NUM_COLUMNS;
	foodY = rand() % (NUM_ROWS - SCORE_LINE_ROW + 1) + (SCORE_LINE_ROW + 1);
	
	// Draw the food
	draw_setPixel(foodX, foodY, COLOR_RED);
}

void runGame(){
	initGameState();
	
	while (1){
		updateGameState(); // Update the game state based on joystick input, collisions, etc.
		
		// Check if button is pressed, to go to the pause menu
		if (js_getButton()){
			pauseMenu();
		}
		
		// Introduce a delay to control the game speed
		_delay_ms(100);
	}
}

void restoreDisplay(){
	// Draw a line to separate the score and the game field
	for (uint8_t i = 0; i < 32; i++){
		draw_setPixel(i, SCORE_LINE_ROW, COLOR_BLUE);
	}
	
	// Draw the score
	draw_number(currentScore, true, 10, 0, COLOR_WHITE, false, false);
	draw_number(highScore, true, 20, 0, COLOR_WHITE, false, false);
	
	// Draw the food
	draw_setPixel(foodX, foodY, COLOR_RED);
	
	// Draw the head of the snake
	draw_setPixel(snakeHeadX, snakeHeadY, COLOR_GREEN);
	
	// Initialize auxiliary variables
	uint8_t bitPair;
	uint8_t bodyX = snakeHeadX;
	uint8_t bodyY = snakeHeadY;
	uint16_t head = snakeBuffer.head;
	uint16_t tail = snakeBuffer.tail;
	
	// Calculate the coordinates of the tail, tracing the path from the head to the tail
	while (head != tail){
		bitPair = getBufferBitPair(head);
		switch (bitPair){
			case RIGHT:
				bodyX--;
				break;
			case LEFT:
				bodyX++;
				break;
			case UP:
				bodyY++;
				break;
			case DOWN:
				bodyY--;
				break;
		}
		draw_setPixel(bodyX, bodyY, COLOR_GREEN);
		head = (head - 2) % RINGBUFFER_SIZE;
	}
}

void gameOver(){
	// Display game over message and scores
	draw_clearDisplay();
	
	// Draw GAME
	draw_letter('G', 5, 3, COLOR_RED, false, true);
	draw_letter('A', 11, 3, COLOR_RED, false, true);
	draw_letter('M', 17, 3, COLOR_RED, false, true);
	draw_letter('E', 23, 3, COLOR_RED, false, true);
	
	// Draw OVER
	draw_letter('O', 5, 11, COLOR_RED, false, true);
	draw_letter('V', 11, 11, COLOR_RED, false, true);
	draw_letter('E', 17, 11, COLOR_RED, false, true);
	draw_letter('R', 23, 11, COLOR_RED, false, true);
	
	// Draw curr
	draw_letter('c', 5, 19, COLOR_GREEN, false, false);
	draw_letter('u', 9, 19, COLOR_GREEN, false, false);
	draw_letter('r', 13, 19, COLOR_GREEN, false, false);
	draw_letter('r', 17, 19, COLOR_GREEN, false, false);
	
	draw_number(currentScore, false, 22, 19, COLOR_WHITE, false, false);
	
	// Draw high
	draw_letter('h', 5, 25, COLOR_GREEN, false, false);
	draw_letter('i', 9, 25, COLOR_GREEN, false, false);
	draw_letter('g', 13, 25, COLOR_GREEN, false, false);
	draw_letter('h', 17, 25, COLOR_GREEN, false, false);
	
	draw_number(highScore, false, 22, 25, COLOR_WHITE, false, false);
	
	// Wait for joystick button press to restart
	while (!js_getButton()){
		// Do nothing, wait for the button press
	}
	
	os_waitForNoJoystickButtonInput();
	// Clear the LED matrix and restart the game
	initGameState();
}

void pauseMenu(){
	os_waitForNoJoystickButtonInput();
	draw_clearDisplay();
	
	draw_letter('P', 2, 10, COLOR_YELLOW, false, true);
	draw_letter('A', 8, 10, COLOR_YELLOW, false, true);
	draw_letter('U', 14, 10, COLOR_YELLOW, false, true);
	draw_letter('S', 20, 10, COLOR_YELLOW, false, true);
	draw_letter('E', 26, 10, COLOR_YELLOW, false, true);
	
	// Draw curr
	draw_letter('c', 5, 19, COLOR_GREEN, false, false);
	draw_letter('u', 9, 19, COLOR_GREEN, false, false);
	draw_letter('r', 13, 19, COLOR_GREEN, false, false);
	draw_letter('r', 17, 19, COLOR_GREEN, false, false);
	
	draw_number(currentScore, false, 22, 19, COLOR_WHITE, false, false);
	
	// Draw high
	draw_letter('h', 5, 25, COLOR_GREEN, false, false);
	draw_letter('i', 9, 25, COLOR_GREEN, false, false);
	draw_letter('g', 13, 25, COLOR_GREEN, false, false);
	draw_letter('h', 17, 25, COLOR_GREEN, false, false);
	
	draw_number(highScore, false, 22, 25, COLOR_WHITE, false, false);
	
	// Wait for joystick button press to resume or restart
	while (1){
		if (js_getButton()){
			if (js_getDirection() == JS_NEUTRAL){
				os_waitForNoJoystickButtonInput();
				draw_clearDisplay();
				restoreDisplay();
				break; // Resume the game
			}
			else{
				// Restart the game
				initGameState();
				break;
			}
		}
	}
}
