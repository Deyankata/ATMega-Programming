/*
 * led_snake.h
 *
 * Created: 14.01.2024 12:51:41
 *  Author: bx541002
 */ 

#ifndef LED_SNAKE_H_
#define LED_SNAKE_H_

#include <stdbool.h>
#include "joystick.h"

#define NUM_COLUMNS 32
#define NUM_ROWS 32

#define MAX_SNAKE_LENGTH 1024
#define RINGBUFFER_SIZE (2 * MAX_SNAKE_LENGTH) 

#define SCORE_LINE_ROW 5

#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3

typedef struct {
	uint16_t head;
	uint16_t tail;
	uint8_t directions[256];
} RingBuffer;


// Function to initialize the snake at the start of the game
void initSnake(uint8_t startX, uint8_t startY);

// Function to move the snake in the current direction
void moveSnake(void);

// Function to check for collisions with the walls or itself
bool checkCollision(void);

// Function to check if the snake has eaten the food
bool checkFoodEaten(uint8_t foodX, uint8_t foodY);

// Function to initialize the game state
void initGameState(void);

// Function that updates the snake direction, using an encoded direction
void updateSnakeDirection(Direction newDirection);

// Function that calculates the next element of the ring buffer
void updateRingBuffer(void);

// Function that returns an element at a specific position in the buffer
uint8_t getBufferBitPair(uint16_t bitPairIndex);

// Function that draws the current state of the snake
void drawSnake(void);

// Function to update the game state based on joystick input
void updateGameState(void);

// Function to generate a random position for the food
void generateFood(void);

void restoreDisplay(void);

// Function to run the main game loop
void runGame(void);

// Function to handle game over and restart
void gameOver(void); 

// Function to handle the pause menu
void pauseMenu(void);

#endif /* LED_SNAKE_H_ */