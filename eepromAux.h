/*Learning TV Remote Kit Code
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
 * eepromAux.h
 *
 * Created: 05/07/2015 18:59:09
 *  Author: Marcus Herbert
 */ 

#include <avr/io.h>
#include <avr/eeprom.h>
#include "defines.h"

#define EEPROM_MAXIMUM 512
#define EEPROM_SIGNATURE 0x4F


#define SIG_LOW_TIME_START 1
#define SIG_LOW_TIME_END (SIG_LOW_TIME_START + 2*TIME_ALLOCATED_WORDS - 1)

#define SIG_LOW_HUFF_START (SIG_LOW_TIME_END + 1)
#define SIG_LOW_HUFF_END (SIG_LOW_HUFF_START + 2*HUFFMAN_ALLOCATED_WORDS - 1)

#define SIG_HIGH_TIME_START (SIG_LOW_HUFF_END + 1)
#define SIG_HIGH_TIME_END (SIG_HIGH_TIME_START + 2*TIME_ALLOCATED_WORDS - 1)

#define SIG_HIGH_HUFF_START (SIG_HIGH_TIME_END + 1)
#define SIG_HIGH_HUFF_END (SIG_HIGH_HUFF_START + 2*HUFFMAN_ALLOCATED_WORDS - 1)

#define SIG_LOW_FREQ_START (SIG_HIGH_HUFF_END + 1)
#define SIG_LOW_FREQ_END (SIG_LOW_FREQ_START + TIME_ALLOCATED_WORDS - 1) //This is in bytes

#define SIG_HIGH_FREQ_START (SIG_LOW_FREQ_END + 1)
#define SIG_HIGH_FREQ_END (SIG_HIGH_FREQ_START + TIME_ALLOCATED_WORDS - 1) //This is in bytes

#define EEPROM_OFFSET (6*TIME_ALLOCATED_WORDS + 4*HUFFMAN_ALLOCATED_WORDS) //This should be 100
//Simply do (SIG_LOW_TIME_START + n*OFFSET) to get the nth start position

/* Saves all time values and associated Huffman streams for one signal to EEPROM */
void save_to_eeprom(uint16_t *lowWords, uint16_t *lowStream, uint16_t *highWords, uint16_t *highStream, uint8_t signalNumber);

/* Iterates over an array of words and saves each word to eeprom */
void eeprom_byte_iterate(uint8_t *words, uint16_t start, uint16_t finish, uint8_t readOrWrite);

/* Saves all low time values and associated Huffman streams for one signal to EEPROM */
void eeprom_save_low_signal(uint16_t *lowWords, uint16_t *lowStream, uint8_t *frequencies, uint8_t signalNumber);

/* Saves all high time values and associated Huffman streams for one signal to EEPROM */
void eeprom_save_high_signal(uint16_t *highWords, uint16_t *highStream, uint8_t *frequencies, uint8_t signalNumber);

/* Populates the low signal arrays with retrieved data from EEPROM */
void eeprom_retrieve_low_signal(uint16_t *lowWords, uint16_t *lowStream, uint8_t *frequencies, uint8_t signalNumber);

/* Populates the high signal arrays with retrieved data from EEPROM */
void eeprom_retrieve_high_signal(uint16_t *highWords, uint16_t *highStream, uint8_t *frequencies, uint8_t signalNumber);

/* Check the signature in EEPROM, wipe EEPROM if signature is absent and put signature in */
void check_signature(void);

/* Prints out a single array from start to finish */
//void print_uint16_t_array(uint16_t start, uint16_t finish, uint16_t *array);

/* Print out stored EEPROM values */
//void print_eeprom(uint8_t signalNumber);

/* Iterates over an array of words and saves each word to eeprom */
void eeprom_iterate(uint16_t *words, uint16_t start, uint16_t finish, uint8_t readOrWrite);

/* Populates the appropriate arrays with retrieved data from EEPROM */
void eeprom_retrieve_signal(uint16_t *lowWords, uint16_t *lowStream, uint16_t *highWords, uint16_t *highStream, uint8_t signalNumber);
