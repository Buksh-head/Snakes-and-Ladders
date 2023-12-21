/*
 * project.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett
 * Modified by <Adnaan Buksh>
 */ 


#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "project.h"
#include "game.h"
#include "display.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);

volatile uint8_t seven_seg_cc = 0;
int rolling;
int board_type;
int count;
int start;
int turn;
int turn2;
int multi;
int p1;
int p1_limit;
int p2_limit;
int limit;
int time_limit;
int p1_limit_sec;
int p2_limit_sec;
int p1_minus;
int p2_minus;
int pause;
int sound_on_off;


uint16_t value;
uint8_t x_or_y = 0;
int x;
int y;


uint8_t turn_data[10] = {63,6,91,79,102,109,125,7,127,111};
uint8_t seven_seg_data[6] = {6,91,79,102,109,125};
uint32_t last_flash_time, current_time, current_time2, last_dice_time, last_switch,switch_player, pause_offset;
uint8_t btn; // The button pushed

void play_sound(){
	if (sound_on_off == 0){
		OCR1A = (20*(1000000UL / 8000)-1);
		_delay_ms(1000);
		OCR1A = (40*(1000000UL / 9000)-1);
		_delay_ms(100);
		OCR1A = (90*(1000000UL / 10000)-1);
		_delay_ms(100);
	OCR1A =0;}
}

/////////////////////////////// main //////////////////////////////////
int main(void) {
	
	OCR1B = 0xFF;
	TCCR1A = (1 << WGM10) | (1 << WGM11) | (1 << CS10) | (1 << COM1A1)| (1 << COM1B1);
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11) ;
	OCR1A = 0;
	
	sei();
	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
	
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete.
	start_screen();
	// Loop forever and continuously play the game.
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	init_timer0();
	
	// Turn on global interrupts
	sei();
}


void start_screen(void) {
	// Clear terminal screen and output a message
	sound_on_off =0;
	sound_on();
	clear_terminal();
	move_terminal_cursor(10,10);
	printf_P(PSTR("Snakes and Ladders"));
	move_terminal_cursor(10,12);
	printf_P(PSTR("CSSE2010/7201 A2 by Adnaan Buksh - 47435568"));
	move_terminal_cursor(10,14);
	printf_P(PSTR("Board Chosen: One"));
	move_terminal_cursor(10,16);
	printf_P(PSTR("One Player"));
	move_terminal_cursor(10,8);
	printf_P(PSTR("Sound ON "));

	// Output the static start screen and wait for a push button 
	// to be pushed or a serial input of 's'
	start_display();
	
	// Wait until a button is pressed, or 's' is pressed on the terminal
	while(1) {	
		// First check for if a 's' is pressed
		// There are two steps to this
		// 1) collect any serial input (if available)
		// 2) check if the input is equal to the character 's'
		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		// If the serial input is 's', then exit the start screen
		if (serial_input == '2'){
			multi = 1;
			activate_multiplayer();
			move_terminal_cursor(10,16);
			printf_P(PSTR("Two Player"));
			move_terminal_cursor(10,18);
			printf_P(PSTR("Easy: No time limit          "));
		}
		if (serial_input == '1'){
			multi =0;
			deactivate_multiplayer();
			move_terminal_cursor(10,16);
			printf_P(PSTR("One Player"));
			move_terminal_cursor(10,18);
			printf_P(PSTR("                             "));
		}
		if (serial_input == 's' || serial_input == 'S') {
			break;
		}
		if ((serial_input == 'e' || serial_input == 'E') && multi == 1) {
			move_terminal_cursor(10,18);
			printf_P(PSTR("Easy: No time limit          "));
			limit = 0;
		}
		if ((serial_input == 'm' || serial_input == 'M') && multi == 1) {
			move_terminal_cursor(10,18);
			printf_P(PSTR("Medium: 90 seconds time limit"));
			time_limit = 90;
			p1_limit_sec =10;
			p2_limit_sec =10;
			limit = 1;
		}
		if ((serial_input == 'h' || serial_input == 'H') && multi == 1) {
			move_terminal_cursor(10,18);
			printf_P(PSTR("Hard: 45 seconds time limit  "));
			time_limit = 45;
			p1_limit_sec =10;
			p2_limit_sec =10;
			limit = 1;
		}
		if (serial_input == 'b' || serial_input == 'B') {
			board_type = 1^board_type;
			move_terminal_cursor(10,14);
			if (board_type==0){printf_P(PSTR("Board Chosen: One"));}
			if (board_type==1){printf_P(PSTR("Board Chosen: Two"));}
			choose_board(board_type);
		}
		if (serial_input == 'q' || serial_input == 'Q') {
			sound_on_off = 1 ^ sound_on_off;
			if (sound_on_off == 1){
				move_terminal_cursor(10,8);
				printf_P(PSTR("Sound OFF"));
				sound_off();}
			if (sound_on_off == 0){
				move_terminal_cursor(10,8);
				printf_P(PSTR("Sound ON "));
				sound_on();}
			
		}
		
		// Next check for any button presses
		int8_t btn = button_pushed();
		if (btn != NO_BUTTON_PUSHED) {
			break;
		}
		
	}
	
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the game and display
	initialise_game();
	DDRC =0xFF;
	DDRD |= (1<<DDRD2);
	PORTC = 63;
	turn2 = 0;
	turn = 0;
	start = 0;
	rolling =0;
	p1 =0;
	sound_on_off=0;
	pause_offset =0;
	x=500;
	y=500;
	value=500;
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
}

void switch_ssd(void){
	if (seven_seg_cc == 0 && start == 0){
		PORTC = 63;
		_delay_ms(5);
		seven_seg_cc = 1 ^ seven_seg_cc;
	}
	else if (seven_seg_cc == 0 ){
		PORTC = seven_seg_data[count];
		PORTC |= (seven_seg_cc<<PINC7);
		_delay_ms(5);
		seven_seg_cc = 1 ^ seven_seg_cc;
	}
	else if (seven_seg_cc == 1 && p1 == 0){
		PORTC = turn_data[turn];
		PORTC |= (seven_seg_cc<<PINC7);
		_delay_ms(5);
	seven_seg_cc = 1 ^ seven_seg_cc;}

	
	else if (seven_seg_cc == 1 && p1 == 1){
		PORTC = turn_data[turn2];
		PORTC |= (seven_seg_cc<<PINC7);
		_delay_ms(5);
	seven_seg_cc = 1 ^ seven_seg_cc;}
}

void delay_move(){
	int c = get_current_time();
	int d =get_current_time();
	while (c <= d+150)
	{c = get_current_time();
		switch_ssd();
	}
}

void play_game(void) {
	last_dice_time = get_current_time();
	last_flash_time = get_current_time();
	last_switch = get_current_time();
	p1_limit = time_limit;
	p2_limit = time_limit;
	// We play the game until it's over
	while(!is_game_over()) {
		
		if(x_or_y == 0) {
			ADMUX &= ~1;
			} else {
			ADMUX |= 1;
		}

		ADCSRA |= (1<<ADSC);
		
		while(ADCSRA & (1<<ADSC)) {
			;
		}
		value = ADC;
		if(x_or_y == 0) {
			//printf("X: %4d ", value);
			x = value;
			} 
		if(x_or_y == 1) {
			y = value;
			//printf("Y: %4d\n", value);
		}
		
		x_or_y ^= 1;

		if (x >= 900 && y > 420 && y < 600 ){
			x=500;
			y=500;
			change_joystick();
			move_player(0,1);
			delay_move();
			change_joystick();	
		}
		if (x <= 100 && y > 420 && y < 600 ){
			x=500;
			y=500;
			change_joystick();
			move_player(0,-1);
			delay_move();
			change_joystick();
		}
		if (y >= 900 && x > 420 && x < 600 ){
			x=500;
			y=500;
			change_joystick();
			move_player(-1,0);
			delay_move();
			change_joystick();
		}
		if (y <= 100 && x > 420 && x < 600 ){
			x=500;
			y=500;
			change_joystick();
			move_player(1,0);
			delay_move();
			change_joystick();
		}
			
		if (x > 832 &&  y < 190 ){
			x=500;
			y=500;
			change_joystick();
			move_player(1,1);
			delay_move();
			change_joystick();
		}
		if (x > 832 && y > 820){
			x=500;
			y=500;
			change_joystick();
			move_player(-1,1);
			delay_move();
			change_joystick();
		}
		if (x < 190 && y < 190 ){
			x=500;
			y=500;
			change_joystick();
			move_player(1,-1);
			delay_move();
			change_joystick();
		}
		if (x < 190 && y > 820 ){
			x=500;
			y=500;
			change_joystick();
			move_player(-1,-1);
			delay_move();
			change_joystick();
		}		
		
		DDRD |= (1<<DDRD4);		
		// We need to check if any button has been pushed, this will be
		// NO_BUTTON_PUSHED if no button has been pushed
		btn = button_pushed();
		
		if (btn == BUTTON0_PUSHED) {
			// If button 0 is pushed, move the player 1 space forward
			// YOU WILL NEED TO IMPLEMENT THIS FUNCTION
			move_player_n(1);
			last_flash_time = get_current_time();
			if(multi == 1){
				if (p1 == 0){
					turn += 1;
					_delay_ms(100);
					p1 = p1^1;
				}
				else{turn2 += 1;
					_delay_ms(100);
				p1 = p1^1;}
			}
			else{turn += 1;}
		}
		// ADDED CODE >
		else if (btn == BUTTON1_PUSHED) {
			// If button 0 is pushed, move the player 1 space forward
			// YOU WILL NEED TO IMPLEMENT THIS FUNCTION
			move_player_n(2);
			last_flash_time = get_current_time();
			if(multi == 1){
				if (p1 == 0){
					turn += 1;
					_delay_ms(100);
					p1 = p1^1;
				}
				else{turn2 += 1;
					_delay_ms(100);
				p1 = p1^1;}
			}
			else{turn += 1;}
		}
	
		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		
		if (serial_input == 'q' || serial_input == 'Q') {
			sound_on_off = 1 ^ sound_on_off;
			if (sound_on_off == 1){
				move_terminal_cursor(10,8);
				printf_P(PSTR("Sound OFF"));
				sound_off();}
			if (sound_on_off == 0){
				sound_on();
				move_terminal_cursor(10,8);
				printf_P(PSTR("Sound ON "));
			}
		}
		
		if (serial_input == 'p' || serial_input == 'P' || btn == BUTTON3_PUSHED) {
			pause_offset = get_current_time();
			while(pause != 1){	
				char serial_input = -1;
				if (serial_input_available()) {
					serial_input = fgetc(stdin);
				}
				
				if (serial_input == 'p' || serial_input == 'P') {
					pause_offset =  get_current_time() - pause_offset;
					pause = 1;
				}
				
				if (btn == button_pushed()){btn = NO_BUTTON_PUSHED;}
				
				if (seven_seg_cc == 0 && start == 0){
					PORTC = 63;
					_delay_ms(5);
					seven_seg_cc = 1 ^ seven_seg_cc;
				}
				else if (seven_seg_cc == 0 ){
					PORTC = seven_seg_data[count];
					PORTC |= (seven_seg_cc<<PINC7);
					_delay_ms(5);
					seven_seg_cc = 1 ^ seven_seg_cc;
				}
				else if (seven_seg_cc == 1 && p1 == 0){
					PORTC = turn_data[turn];
					PORTC |= (seven_seg_cc<<PINC7);
					_delay_ms(5);
				seven_seg_cc = 1 ^ seven_seg_cc;}

				
				else if (seven_seg_cc == 1 && p1 == 1){
					PORTC = turn_data[turn2];
					PORTC |= (seven_seg_cc<<PINC7);
					_delay_ms(5);
				seven_seg_cc = 1 ^ seven_seg_cc;}
			}
			pause=0;
		}
		
		
		if ((serial_input == 'e' || serial_input == 'E') && multi == 1) {
			move_terminal_cursor(10,17);
			printf_P(PSTR("                             "));
			move_terminal_cursor(10,18);
			printf_P(PSTR("Easy: No time limit          "));
			limit = 0;
		}
		if ((serial_input == 'm' || serial_input == 'M') && multi == 1) {
			move_terminal_cursor(10,18);
			printf_P(PSTR("Medium: 90 seconds time limit"));
			time_limit = 90;
			p1_limit = time_limit;
			p2_limit =time_limit;
			p1_limit_sec =10;
			p2_limit_sec =10;
			p1_minus =0;
			p2_minus =0;
			limit = 1;
			
		}
		if ((serial_input == 'h' || serial_input == 'H') && multi == 1) {
			move_terminal_cursor(10,18);
			printf_P(PSTR("Hard: 45 seconds time limit  "));
			time_limit = 45;
			p1_limit = time_limit;
			p2_limit =time_limit;
			p1_limit_sec =10;
			p2_limit_sec =10;
			p1_minus =0;
			p2_minus =0;
			limit = 1;
			
		}
		if (serial_input == 'w' || serial_input == 'W') {
			move_player(0,1);
			last_flash_time = get_current_time();
		}
		if (serial_input == 'a' || serial_input == 'A') {
			move_player(-1,0);
			last_flash_time = get_current_time();
		}
		if (serial_input == 's' || serial_input == 'S') {
			move_player(0,-1);
			last_flash_time = get_current_time();
		}
		if (serial_input == 'd' || serial_input == 'D') {
			move_player(1,0);
			last_flash_time = get_current_time();
		}
		if (serial_input == 'r' || serial_input == 'R'|| (btn == BUTTON2_PUSHED)) {
			start = 1;
			if (rolling == 0){
				PIND |= (1<<PIND2);
				move_terminal_cursor(10,5);
				printf_P(PSTR("Dice status: Rolling    "));
				move_terminal_cursor(10,6);
				printf_P(PSTR("Last Roll: %d"),count+1 );
				rolling = 1;				
			}
			else if (rolling == 1){
				if(multi == 1){
					if (p1 == 0){
						turn += 1;
						_delay_ms(100);
						p1 = p1^1;
					}
					else{turn2 += 1;
						_delay_ms(100);
						p1 = p1^1;}
				}
				else{turn += 1;}
				
				PIND |= (1<<PIND2);
	
				move_terminal_cursor(10,5);
				printf_P(PSTR("Dice status: Not rolling"));
				move_terminal_cursor(10,6);
				printf_P(PSTR("Last Roll: %d"),count+1 );
				
				move_player_n(count +1);
				rolling = 0;	
			}

		}
		// MY CODE ABOVE
		
		current_time = get_current_time();
		current_time2 = get_current_time();
		switch_player = get_current_time();
		if (p1_limit >= 10 && get_cur_player() == 0 && limit == 1 && switch_player >= last_switch + 500){
			p1_limit = p1_limit - p1_minus;
			move_terminal_cursor(10,17);
			printf_P(PSTR("Player 1: %d Seconds left"),p1_limit);
			last_switch = switch_player;
			p1_minus = 1^p1_minus;
		}
		if (p2_limit >= 10 && get_cur_player() == 1 && limit == 1 && switch_player >= last_switch + 500){
			p2_limit = p2_limit - p2_minus;
			move_terminal_cursor(10,17);
			printf_P(PSTR("Player 2: %d Seconds left"),p2_limit);
			last_switch = switch_player;
			p2_minus = 1^p2_minus;
		}
		if (p1_limit < 10 && get_cur_player() == 0 && limit == 1 && switch_player >= last_switch + 100){
			p1_limit_sec = p1_limit_sec - 0.10;
			move_terminal_cursor(10,17);
			printf_P(PSTR("Player 1: %d.%d Seconds left"),p1_limit,p1_limit_sec);
			last_switch = switch_player;
			if (p1_limit_sec == 0 && p1_limit != 0)
			{p1_limit_sec =10;
				p1_limit -= 1;
			}
		}
		if (p2_limit < 10 && get_cur_player() == 1 && limit == 1 && switch_player >= last_switch + 100){
			p2_limit_sec = p2_limit_sec - 0.10;
			move_terminal_cursor(10,17);
			printf_P(PSTR("Player 2: %d.%d Seconds left"),p2_limit,p2_limit_sec);
			last_switch = switch_player;
			if (p2_limit_sec == 0 && p2_limit != 0)
			{p2_limit_sec =10;
			p2_limit -= 1;
			}
		}
		if (((p1_limit <= 0 && p1_limit_sec == 0)||(p2_limit <= 0 && p2_limit_sec==0)) && limit == 1 && multi == 1){
			handle_game_over();	
		}
		
		if (current_time >= last_flash_time + pause_offset + 500) {
			// 500ms (0.5 second) has passed since the last time we
			// flashed the cursor, so flash the cursor
			flash_player_cursor();
			// Update the most recent time the cursor was flashed
			last_flash_time = current_time;
			
			if (pause_offset != 0 ){
				pause_offset =0;
			}
		}
		if (current_time2 >= last_dice_time + 62 && rolling == 1){
			count += 1;
			if (count == 6){
				count = 0;
			}
			last_dice_time = current_time2;
		}
		
	if (turn == 10){
		turn =0;
	}
	if (turn2 == 10){
		turn2 =0;
	}
	switch_ssd();
}
}
 
 
void handle_game_over() {
	move_terminal_cursor(10,17);
	if (get_winner() == 1){
		printf_P(PSTR("GAME OVER: Player 1 Wins (Orange)"));
	}
	else if (get_winner() == 2){
		printf_P(PSTR("GAME OVER: Player 2 Wins (Yellow)"));}

		
	move_terminal_cursor(10,18);
	printf_P(PSTR("Press a button to start again"));
	
	PORTC =0x00;
			
	while(button_pushed() == NO_BUTTON_PUSHED ) {
		show_winner();
		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		if (serial_input == 'q' || serial_input == 'Q') {
			sound_on_off = 1 ^ sound_on_off;
			if (sound_on_off == 1){
			sound_off();}
			if (sound_on_off == 0){
				sound_on();
			}
		}
		if (serial_input == 's' || serial_input == 'S'){
			deactivate_multiplayer();
			main();
		}
		
	}
	deactivate_multiplayer();
	main();
	
}
