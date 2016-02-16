/*
Learning TV Remote Kit Code
    Copyright (C) 2016  Marcus Herbert

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

/*
 * remoteControl.h
 *
 * Created: 05/07/2015 23:59:00
 *  Author: Marcus Herbert
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "defines.h"

/* Initialises the watchdog timer */
void init_watchdog_timer(void);

/* Resets and turns off the watchdog timer */
void turn_off_WDT(void);

/* Starts timer 0 */
void start_timer0(void);

/* Stops timer 0 */
void stop_timer0(void);

/* Initialise the timer0 interrupt */
void init_timer0(void);

/* Polls the buttons that signals can be assigned to */
void check_rx_buttons(void);

/* Sets timer0 up to interrupt every 5 microseconds */
void init_timer1(void);

/* Initialises the hardware for the device */
void init_hardware(void);

/* Initialises UART0 for serial transfer */
void init_serial(void);

/* Prints out a single array from start to finish */
void print_uint16_t_array(uint16_t start, uint16_t finish, uint16_t *array);

/* Initialises USART TX */
void usart_transmit(unsigned char byte);

/*Initialises the external interrupt on int0 */
void init_external_interrupt(void);

/* Prints out the data and resets parameters for a new receiving */
uint8_t receive_complete(void);

/* Initialise Timer 2 for the output waveform */
void init_timer2(void);

/* Starts the waveform generation timer */
void start_timer2(void);

/* Stops the waveform generation timer */
void stop_timer2(void);

/* Send the currently saved data out */
void send_data(void);

/* Disable int0 to prevent data erasing */
void disable_int0(void);

/* Enable int0 to allow data to be received */
void enable_int0(void);

/* Sort the received values */
void array_sorter(void);

/* Prints out an unsigned 16 bit integer */
void print_uint16_t(uint16_t number);

/* Moves to the start of the next line */
void next_line(void);

/* Prints a spaced line */
void line_break(void);

/* Prints the binary representation of a uint16_t number from a certain start bit */
void print_binary(uint16_t num, uint16_t startBit);

/* Prints out a string */
void print_string(char *string);

/* Enable the signal buttons (PCINT) */
void enable_pcint1(void);

/* Disable the signal buttons (PCINT) */
void disable_pcint1(void);

/* Put the microcontroller to sleep to conserve power */
void sleep(void);
