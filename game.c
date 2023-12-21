/*
 * game.c
 *
 * Functionality related to the game state and features.
 *
 * Author: Jarrod Bennett
 */ 


#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "display.h"
#include "terminalio.h"
#include "timer0.h"
#include "project.h"


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

int on_off_sound;
int stick;

uint8_t board[WIDTH][HEIGHT];

// The initial game layout. Note that this is laid out in such a way that
// starting_layout[x][y] does not correspond to an (x,y) coordinate but is a
// better visual representation (but still somewhat messy).
// In our reference system, (0,0) is the bottom left, but (0,0) in this array
// is the top left.
static const uint8_t starting_layout[HEIGHT][WIDTH] =
{
	{FINISH_LINE, 0, 0, 0, 0, 0, 0, 0},
	{0, SNAKE_START | 4, 0, 0, LADDER_END | 4, 0, 0, 0},
	{0, SNAKE_MIDDLE | 4, 0, LADDER_MIDDLE | 4, 0, 0, 0, 0},
	{0, SNAKE_MIDDLE | 4, LADDER_START | 4, 0, 0, 0, 0, 0},
	{0, SNAKE_END | 4, 0, 0, 0, 0, SNAKE_START | 3, 0},
	{0, 0, 0, 0, LADDER_END | 3, 0, SNAKE_MIDDLE | 3, 0},
	{SNAKE_START | 2, 0, 0, 0, LADDER_MIDDLE | 3, 0, SNAKE_MIDDLE | 3, 0},
	{0, SNAKE_MIDDLE | 2, 0, 0, LADDER_START | 3, 0, SNAKE_END | 3, 0},
	{0, 0, SNAKE_END | 2, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, SNAKE_START | 1, 0, 0, 0, LADDER_END | 1},
	{0, LADDER_END | 2, 0, SNAKE_MIDDLE | 1, 0, 0, LADDER_MIDDLE| 1, 0},
	{0, LADDER_MIDDLE | 2, 0, SNAKE_MIDDLE | 1, 0, LADDER_START | 1, 0, 0},
	{0, LADDER_START | 2, 0, SNAKE_MIDDLE | 1, 0, 0, 0, 0},
	{START_POINT, 0, 0, SNAKE_END | 1, 0, 0, 0, 0}
};

static const uint8_t starting_layout2[HEIGHT][WIDTH] =
{
	{FINISH_LINE, 0, 0, SNAKE_START | 4, 0, 0, 0, LADDER_END | 4},
		      {0, 0, 0, SNAKE_MIDDLE| 4, 0, 0, LADDER_MIDDLE| 4, 0},
			  {0, 0, 0, SNAKE_MIDDLE| 4, 0, LADDER_START | 4, 0, 0},
			  {0, 0, 0, SNAKE_END | 4, 0, 0, 0, 0},
			  {0, 0, 0, 0, 0, 0, 0, 0},
	   		  {SNAKE_START | 3, 0, 0, 0, LADDER_END | 3, 0, 0, 0},
			  {0, SNAKE_MIDDLE| 3, 0, 0, LADDER_MIDDLE| 3, 0, 0, 0},
			  {0, 0, SNAKE_END | 3, 0, LADDER_MIDDLE| 3, 0, 0, 0},
			  {0, 0, 0, 0, LADDER_START | 3, 0, 0, 0},
			  {0, 0, 0, 0, 0, 0, 0, 0},
			  {0, 0, 0, SNAKE_START | 2, 0, 0, 0, 0},
			  {LADDER_END | 2, 0, 0, SNAKE_MIDDLE| 2, 0, 0, 0, 0},
			  {LADDER_MIDDLE| 2, 0, 0, SNAKE_END | 2, 0,0, LADDER_END | 1, 0},
			  {LADDER_START | 2, 0, SNAKE_START | 1,0,0,0, LADDER_MIDDLE| 1, 0},
			  {0, 0, 0, SNAKE_MIDDLE| 1, 0, 0, LADDER_START | 1,0},
	{START_POINT, 0, 0,0, SNAKE_END | 1, 0, 0, 0}
};
static const uint8_t p1_winner[HEIGHT][WIDTH] =
{
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, PLAYER_1, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, 0, 0, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, 0, 0, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, 0, 0, PLAYER_1, PLAYER_1, 0, 0, 0},
	{0, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, 0},
	{0, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, PLAYER_1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}
};

static const uint8_t p2_winner[HEIGHT][WIDTH] =
{
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, 0},
	{0, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, 0},
	{0, PLAYER_2, PLAYER_2, 0, 0, PLAYER_2, PLAYER_2, 0},
	{0, 0, 0, 0, 0, PLAYER_2, PLAYER_2, 0},
	{0, 0, 0, 0, PLAYER_2, PLAYER_2, 0, 0},
	{0, 0, 0, PLAYER_2, PLAYER_2, 0, 0, 0},
	{0, 0, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, 0},
	{0, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, PLAYER_2, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}
};

// The player is not stored in the board itself to avoid overwriting game
// elements when the player is moved.
int8_t player_1_x;
int8_t player_1_y;
int8_t player_2_x;
int8_t player_2_y;
int multiplayer;
int player_x;
int player_y;
int player[2] = {PLAYER_1,PLAYER_2};
int winner;
int board_num; 
int direction;
int a;
int b;
uint8_t current_player = 0;
// For flashing the player icon
uint8_t player_visible;

void activate_multiplayer(void){
	multiplayer = 1; 
}
void deactivate_multiplayer(void){
	multiplayer = 0;
}

void choose_board(uint8_t board_type){
	board_num = board_type;
	initialise_display();
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			if (board_num==0){
				board[x][y] = starting_layout[HEIGHT - 1 - y][x];
			}
			if (board_num==1){
				board[x][y] = starting_layout2[HEIGHT - 1 - y][x];
			}	
			update_square_colour(x, y, get_object_type(board[x][y]));
			
		}
	}
}

void initialise_game(void) {
		
	OCR1B = 0xFF;
	TCCR1A = (1 << WGM10) | (1 << WGM11) | (1 << CS10) | (1 << COM1A1)| (1 << COM1B1);
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11) ;
	OCR1A = 0;
	
	// initialise the display we are using.
	initialise_display();
		
	// start the player icon at the bottom left of the display
	// NOTE: (for INternal students) the LED matrix uses a different coordinate
	// system
	player_1_x = 0;
	player_1_y = 0;
	player_2_x = 0;
	player_2_y = 0;
	stick = 0;
	
	direction = 1;
	player_visible = 0;

	// go through and initialise the state of the playing_field
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			// initialise this square based on the starting layout
			// the indices here are to ensure the starting layout
			// could be easily visualised when declared
			if (board_num==0){
				board[x][y] = starting_layout[HEIGHT - 1 - y][x];
			}
			if (board_num==1){
				board[x][y] = starting_layout2[HEIGHT - 1 - y][x];
			}
			
			update_square_colour(x, y, get_object_type(board[x][y]));
			
		}
	}
	
	update_square_colour(player_1_x, player_1_y, PLAYER_1);
	
}

// Return the game object at the specified position (x, y). This function does
// not consider the position of the player token since it is not stored on the
// game board.
uint8_t get_object_at(uint8_t x, uint8_t y) {
	// check the bounds, anything outside the bounds
	// will be considered empty
	if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
		return EMPTY_SQUARE;
	} else {
		//if in the bounds, just index into the array
		return board[x][y];
	}
}

// Extract the object type of a game element (the upper 4 bits).
uint8_t get_object_type(uint8_t object) {
	return object & 0xF0;
}

// Get the identifier of a game object (the lower 4 bits). Not all objects
// have an identifier, in which case 0 will be returned.
uint8_t get_object_identifier(uint8_t object) {
	return object & 0x0F;
}

// Move the player by the given number of spaces forward.
void move_player_n(uint8_t num_spaces) {
	/* suggestions for implementation:
	 * 1: remove the display of the player at the current location
	 *		(and replace it with whatever object is at that location).
	 * 2: update the positional knowledge of the player, this will include
	 *		variables player_1_x and player_1_y since you will need to consider
	 *		what should happen if the player reaches the end of the current
	 *		row. You will need to consider which direction the player should
	 *		move based on the current row. A loop can be useful to move the
	 *		player one space at a time but is not essential.
	 * 3: display the player at the new location
	 *
	 * FOR "Flashing Player Icon"
	 * 4: reset the player cursor flashing cycle. See project.c for how the
	 *		cursor is flashed.
	 */
	// YOUR CODE HERE
	int i;
	if (current_player ==0){
		player_x = player_1_x;
		player_y = player_1_y;
	}
	if (current_player ==1){
		player_x = player_2_x;
		player_y = player_2_y;
	}
	for(i=0;i<num_spaces;i++){
		uint8_t object_at_cursor = get_object_at(player_x, player_y);
		update_square_colour(player_x, player_y, object_at_cursor);
		
		if (player_y % 2 == 0){
			direction = 1;
		}
		if (player_y % 2 != 0){
			direction = -1;
		}
		
		player_x += direction;
		
		if (player_x >7){
			player_y += 1;
			player_x += -1;
		}
		if (player_x <0){
			
			player_y += 1;
			if (player_y > 15){
				player_x = 0;
				player_y = 15;
				
			}
			else{player_x += 1;}
		}
		if (on_off_sound == 0){OCR1A = ((1000000UL / 25)-1);}
			
		b = get_current_time();
		a = get_current_time();
		
		while (b <= a+40)
		{b = get_current_time();
			switch_ssd();	
		}
		update_square_colour(player_x,player_y,player[current_player]);		
		
		b = get_current_time();
		a = get_current_time();

		while (b <= a+30)
		{b = get_current_time();
			switch_ssd();
		}
		OCR1A = 0;
		if (current_player ==0){
			player_1_x = player_x;
			player_1_y = player_y;
		}
		if (current_player ==1){
			player_2_x = player_x;
			player_2_y = player_y;
		}
	}
	check_snake_ladder();
	is_game_over();
	if (multiplayer == 1){current_player = current_player^1;}
	
	
	// MY CODE ABOVE

}

// Move the player one space in the direction (dx, dy). The player should wrap
// around the display if moved 'off' the display.
void move_player(int8_t dx, int8_t dy) {
	/* suggestions for implementation:
	 * 1: remove the display of the player at the current location
	 *		(and replace it with whatever object is at that location).
	 * 2: update the positional knowledge of the player, this will include
	 *		variables player_1_x and player_1_y and cursor_visible. Make sure
	 *		you consider what should happen if the player moves off the board.
	 *		(The player should wrap around to the current row/column.)
	 * 3: display the player at the new location
	 *
	 * FOR "Flashing Player Icon"
	 * 4: reset the player cursor flashing cycle. See project.c for how the
	 *		cursor is flashed.
	 */	
	// YOUR CODE HERE
	if (current_player ==0){
		player_x = player_1_x;
		player_y = player_1_y;
	}
	if (current_player ==1){
		player_x = player_2_x;
		player_y = player_2_y;
	}
	uint8_t object_at_cursor = get_object_at(player_x, player_y);
	update_square_colour(player_x, player_y, object_at_cursor);
	player_x += dx;
	player_y += dy;
	if (player_y % 2 == 0){
		direction = 1;
	}
	if (player_y % 2 != 0){
		direction = -1;
	}
	if (player_y == -1){
		player_y = 15;
	}
	if (player_y == 16){
		player_y = 0;
	}
	if (player_x == -1){
		player_x = 7;
	}
	if (player_x == 8){
		player_x = 0;
	}
	update_square_colour(player_x,player_y,player[current_player]);
	if (current_player ==0){
		player_1_x = player_x;
		player_1_y = player_y;
	}
	if (current_player ==1){
		player_2_x = player_x;
		player_2_y = player_y;
	}
	if (on_off_sound == 0){	OCR1A = ((1000000UL / 25)-1);}
	b = get_current_time();
	a = get_current_time();
	
	while (b <= a+70)
	{b = get_current_time();
		switch_ssd();
	}
	check_snake_ladder();
	OCR1A = 0;
	is_game_over();
	
	if (multiplayer == 1 && stick == 0){current_player = 1^current_player;}
	//MY CODE ABOVE

}

// Flash the player icon on and off. This should be called at a regular
// interval (see where this is called in project.c) to create a consistent
// 500 ms flash.
void flash_player_cursor(void) {
	if (current_player == 0){
		if(multiplayer ==1){update_square_colour(player_2_x, player_2_y, PLAYER_2);}
		player_x = player_1_x;
		player_y = player_1_y;
	}
	if (current_player == 1){
		update_square_colour(player_1_x, player_1_y, PLAYER_1);
		player_x = player_2_x;
		player_y = player_2_y;
	}
	if (player_visible) {
		// we need to flash the player off, it should be replaced by
		// the colour of the object which is at that location
		uint8_t object_at_cursor = get_object_at(player_x, player_y);
		update_square_colour(player_x, player_y, object_at_cursor);
		
	} else {
		// we need to flash the player on
		update_square_colour(player_x, player_y, player[current_player]);
	}
	player_visible = 1 - player_visible; //alternate between 0 and 1
}

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void) {
	// YOUR CODE HERE
	if (current_player == 0){
		player_x = player_1_x;
		player_y = player_1_y;
	}
	else if (current_player == 1){
		player_x = player_2_x;
		player_y = player_2_y;
	}
	// Detect if the game is over i.e. if a player has won.
	if (get_object_type(get_object_at(player_x,player_y))==FINISH_LINE){
		winner = current_player + 1;
		current_player = 0;
		player_x = 0;
		player_y = 0;
		handle_game_over();
		return 1;
		}
	return 0;
}

void sound(){
	if (on_off_sound == 0){
	OCR1A = (30*(1000000UL / 5000)-1);
	_delay_ms(100);
	OCR1A = (10*(1000000UL / 2500)-1);
	_delay_ms(200);
	OCR1A = ((1000000UL / 200)-1);}
}

void sound_off(){
	on_off_sound = 1;
	OCR1A = 0;
}
void sound_on(){
	on_off_sound = 0;
}

void change_joystick(){
	stick = stick ^ 1;
}

void show_winner(){
	if (winner == 1){
		sound();
		initialise_display();
		for (int x = 0; x < WIDTH; x++) {
			_delay_ms(5);
			for (int y = 0; y < HEIGHT; y++) {
				// initialise this square based on the starting layout
				// the indices here are to ensure the starting layout
				// could be easily visualised when declared
				if (board_num==0){
					board[x][y] = p1_winner[HEIGHT - 1 - y][x];
				}
				if (board_num==1){
					board[x][y] = p1_winner[HEIGHT - 1 - y][x];
				}
				
				update_square_colour(x, y, get_object_type(board[x][y]));
				_delay_ms(5);
				
			}
		}
	}
	
	if (winner == 2){
		sound();
		initialise_display();
		for (int x = 0; x < WIDTH; x++) {
			_delay_ms(5);
			for (int y = 0; y < HEIGHT; y++) {
				// initialise this square based on the starting layout
				// the indices here are to ensure the starting layout
				// could be easily visualised when declared
				if (board_num==0){
					board[x][y] = p2_winner[HEIGHT - 1 - y][x];
				}
				if (board_num==1){
					board[x][y] = p2_winner[HEIGHT - 1 - y][x];
				}
				
				update_square_colour(x, y, get_object_type(board[x][y]));
				_delay_ms(5);
				
			}
		}
	}
}

uint8_t get_winner(){
	if (winner == 0){
		return ((current_player ^ 1) +1);
		  
	}
	else{return winner;}
}

uint8_t get_cur_player(){
	return current_player;
}
	
void check_snake_ladder(void){ 
	if (current_player == 0){
		player_x = player_1_x;
		player_y = player_1_y;
	}
	if (current_player == 1){
		player_x = player_2_x;
		player_y = player_2_y;
	}
	
	uint8_t object_at_cursor = get_object_at(player_x, player_y);
	
		
	if (get_object_type(object_at_cursor) == LADDER_START){
		uint8_t end_point = (get_object_type(LADDER_END) | get_object_identifier(object_at_cursor));
		uint8_t mid_point = (get_object_type(LADDER_MIDDLE) | get_object_identifier(object_at_cursor));
		for (int x = 0; x < WIDTH; x++) {
			for (int y = 0; y <HEIGHT ; y++) {
				if (board[x][y] == mid_point)	{
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+50)
					{b = get_current_time();
						switch_ssd();
					}
					update_square_colour(player_x, player_y, object_at_cursor);
					player_x = x;
					player_y = y;
					update_square_colour(player_x, player_y, player[current_player]);
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+120)
					{b = get_current_time();
						switch_ssd();
					}
				}
				if (board[x][y] == end_point)	{
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+50)
					{b = get_current_time();
						switch_ssd();
					}
					update_square_colour(player_x, player_y, object_at_cursor);
					player_x = x;
					player_y = y;
					update_square_colour(player_x, player_y, player[current_player]);
					if (current_player ==0){
						player_1_x = player_x;
						player_1_y = player_y;
					}
					if (current_player ==1){
						player_2_x = player_x;
						player_2_y = player_y;
					}
					
					break; 
						}
					}
				}
				if (multiplayer == 1 && stick == 1){current_player = current_player^1;}
	}
	if (get_object_type(object_at_cursor) == SNAKE_START){
		uint8_t end_point = (get_object_type(SNAKE_END) | get_object_identifier(object_at_cursor));
		uint8_t mid_point = (get_object_type(SNAKE_MIDDLE) | get_object_identifier(object_at_cursor));
		for (int x = 0; x < WIDTH; x++) {
			for (int y = HEIGHT; y >-1 ; y--) {
				if (board[x][y] == mid_point)	{
					if (on_off_sound == 0){OCR1A = (10*(1000000UL / 4000)-1);}
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+50)
					{b = get_current_time();
						switch_ssd();
					}
					if (on_off_sound == 0){OCR1A = (45*(1000000UL / 5000)-1);}
					update_square_colour(player_x, player_y, object_at_cursor);
					player_x = x;
					player_y = y;
					update_square_colour(player_x, player_y, player[current_player]);
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+120)
					{b = get_current_time();
						switch_ssd();
					}
				}
				if (board[x][y] == end_point)	{
					if (on_off_sound == 0){OCR1A = (40*(1000000UL / 4500)-1);}
					b = get_current_time();
					a = get_current_time();
					
					while (b <= a+50)
					{b = get_current_time();
						switch_ssd();
					}
					update_square_colour(player_x, player_y, object_at_cursor);
					player_x = x;
					player_y = y;
					update_square_colour(player_x, player_y, player[current_player]);
					if (current_player ==0){
						player_1_x = player_x;
						player_1_y = player_y;
					}
					if (current_player ==1){
						player_2_x = player_x;
						player_2_y = player_y;
					}
					OCR1A =0;
					
					break;
					
				}
			}
		}
		if (multiplayer == 1 && stick == 1){current_player = current_player^1;}		
	}
	
}

