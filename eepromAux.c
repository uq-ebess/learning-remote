/* Learning TV Remote Kit Code
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
 * eepromAux.c
 *
 * Created: 05/07/2015 18:59:09
 *  Author: Marcus Herbert
 */ 

#include "eepromAux.h"


/* Check the signature in EEPROM, wipe EEPROM if signature is absent and put signature in */
/* This is currently useless, as it doesn't do anything except add the signature in when the signature is absent */
void check_signature(void) {
	uint8_t signature;
	signature = eeprom_read_byte((uint8_t*) 0);
	if (signature != EEPROM_SIGNATURE) {
		//uint8_t zeros[EEPROM_MAXIMUM]; //Could this blow the stack? //////////////////////////////////////////IMPORTANT
		//for (int i = 0; i < EEPROM_MAXIMUM; i++) zeros[i] = 0;
		//eeprom_update_block((const void*) zeros, (void*) 0, 512); //I think this will work? (I want to wipe everything)
		eeprom_update_byte((uint8_t*) 1, EEPROM_SIGNATURE); //Write the signature in
	}
}

/* Prints out a single array from start to finish */
/*
void print_uint16_t_array(uint16_t start, uint16_t finish, uint16_t *array) {
	for (int i = start; i < finish; i++) {
		print_uint16_t(array[i]);
		usart_transmit('-');
	}
	line_break();
}*/

/* Print out stored EEPROM values */
/*
void print_eeprom(uint8_t signalNumber) {
	uint16_t lowWords[TIME_ALLOCATED_WORDS];
	uint16_t lowStream[HUFFMAN_ALLOCATED_WORDS];
	uint16_t highWords[TIME_ALLOCATED_WORDS];
	uint16_t highStream[HUFFMAN_ALLOCATED_WORDS];
	
	//for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) lowWords[i] = highWords[i] = 0;
	//for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) lowStream[i] = highStream[i] = 0;
	
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(lowWords,	SIG_LOW_TIME_START	+ offset, SIG_LOW_TIME_END	+ offset, 0);
	eeprom_iterate(lowStream,	SIG_LOW_HUFF_START	+ offset, SIG_LOW_HUFF_END	+ offset, 0);
	eeprom_iterate(highWords,	SIG_HIGH_TIME_START + offset, SIG_HIGH_TIME_END + offset, 0);
	eeprom_iterate(highStream,	SIG_HIGH_HUFF_START + offset, SIG_HIGH_HUFF_END + offset, 0);
	
	line_break(); line_break();
	print_uint16_t_array(0, TIME_ALLOCATED_WORDS, lowWords);
	print_uint16_t_array(0, HUFFMAN_ALLOCATED_WORDS, lowStream);
	print_uint16_t_array(0, TIME_ALLOCATED_WORDS, highWords);
	print_uint16_t_array(0, HUFFMAN_ALLOCATED_WORDS, highStream);
}*/

/* Iterates over an array of words and saves each word to eeprom */
void eeprom_iterate(uint16_t *words, uint16_t start, uint16_t finish, uint8_t readOrWrite) {
	if (readOrWrite) { //Write
		for (uint16_t i = start; i <= finish; i+=2){
			eeprom_update_word((uint16_t*) i, words[(i - start)/2]); //Are the addressable blocks 16 bits or 8 bits? Do I move up two addresses at a time with 16 bits?
		}
	} else { //Read
		for (uint16_t i = start; i <= finish; i+=2) {
			words[(i - start)/2] = eeprom_read_word((uint16_t*) i); //Same comment as above
		}
	}
}

/* Iterates over an array of words and saves each word to eeprom */
void eeprom_byte_iterate(uint8_t *words, uint16_t start, uint16_t finish, uint8_t readOrWrite) {
	if (readOrWrite) { //Write
		for (uint16_t i = start; i <= finish; i++){
			eeprom_update_byte((uint8_t*) i, words[i - start]); //Are the addressable blocks 16 bits or 8 bits? Do I move up two addresses at a time with 16 bits?
		}
		} else { //Read
		for (uint16_t i = start; i <= finish; i++) {
			words[i - start] = eeprom_read_byte((uint8_t*) i); //Same comment as above
		}
	}
}

/* Populates the appropriate arrays with retrieved data from EEPROM */
void eeprom_retrieve_signal(uint16_t *lowWords, uint16_t *lowStream, uint16_t *highWords, uint16_t *highStream, uint8_t signalNumber) {
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(lowWords,	SIG_LOW_TIME_START	+ offset, SIG_LOW_TIME_END	+ offset, 0);
	eeprom_iterate(lowStream,	SIG_LOW_HUFF_START	+ offset, SIG_LOW_HUFF_END	+ offset, 0);
	eeprom_iterate(highWords,	SIG_HIGH_TIME_START + offset, SIG_HIGH_TIME_END + offset, 0);
	eeprom_iterate(highStream,	SIG_HIGH_HUFF_START + offset, SIG_HIGH_HUFF_END + offset, 0);
}

/* Saves all time values and associated Huffman streams for one signal to EEPROM */
void save_to_eeprom(uint16_t *lowWords, uint16_t *lowStream, uint16_t *highWords, uint16_t *highStream, uint8_t signalNumber) {
	check_signature();
	
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(lowWords,	SIG_LOW_TIME_START	+ offset, SIG_LOW_TIME_END	+ offset, 1);
	eeprom_iterate(lowStream,	SIG_LOW_HUFF_START	+ offset, SIG_LOW_HUFF_END	+ offset, 1);
	eeprom_iterate(highWords,	SIG_HIGH_TIME_START + offset, SIG_HIGH_TIME_END + offset, 1);
	eeprom_iterate(highStream,	SIG_HIGH_HUFF_START + offset, SIG_HIGH_HUFF_END + offset, 1);
}

/* Saves all low time values and associated Huffman streams for one signal to EEPROM */
void eeprom_save_low_signal(uint16_t *lowWords, uint16_t *lowStream, uint8_t *frequencies, uint8_t signalNumber) {
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(lowWords,	SIG_LOW_TIME_START	+ offset, SIG_LOW_TIME_END	+ offset, 1);
	eeprom_iterate(lowStream,	SIG_LOW_HUFF_START	+ offset, SIG_LOW_HUFF_END	+ offset, 1);
	eeprom_byte_iterate(frequencies,	SIG_LOW_FREQ_START  + offset, SIG_LOW_FREQ_END	+ offset, 1);
}

/* Saves all high time values and associated Huffman streams for one signal to EEPROM */
void eeprom_save_high_signal(uint16_t *highWords, uint16_t *highStream, uint8_t *frequencies, uint8_t signalNumber) {
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(highWords,	SIG_HIGH_TIME_START	+ offset, SIG_HIGH_TIME_END	+ offset, 1);
	eeprom_iterate(highStream,	SIG_HIGH_HUFF_START	+ offset, SIG_HIGH_HUFF_END	+ offset, 1);
	eeprom_byte_iterate(frequencies,	SIG_HIGH_FREQ_START + offset, SIG_HIGH_FREQ_END	+ offset, 1);
}

/* Populates the low signal arrays with retrieved data from EEPROM */
void eeprom_retrieve_low_signal(uint16_t *lowWords, uint16_t *lowStream, uint8_t *frequencies, uint8_t signalNumber) {
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(lowWords,	SIG_LOW_TIME_START	+ offset, SIG_LOW_TIME_END	+ offset, 0);
	eeprom_iterate(lowStream,	SIG_LOW_HUFF_START	+ offset, SIG_LOW_HUFF_END	+ offset, 0);
	eeprom_byte_iterate(frequencies, SIG_LOW_FREQ_START  + offset, SIG_LOW_FREQ_END  + offset, 0);
}

/* Populates the high signal arrays with retrieved data from EEPROM */
void eeprom_retrieve_high_signal(uint16_t *highWords, uint16_t *highStream, uint8_t *frequencies, uint8_t signalNumber) {
	uint16_t offset = signalNumber * EEPROM_OFFSET;
	eeprom_iterate(highWords,	SIG_HIGH_TIME_START	+ offset, SIG_HIGH_TIME_END	+ offset, 0);
	eeprom_iterate(highStream,	SIG_HIGH_HUFF_START	+ offset, SIG_HIGH_HUFF_END	+ offset, 0);
	eeprom_byte_iterate(frequencies, SIG_HIGH_FREQ_START  + offset, SIG_HIGH_FREQ_END  + offset, 0);
}
