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
 * huff.h
 *
 * Created: 5/07/2015 22:53:00
 *  Author: Marcus Herbert
 */ 

#include "defines.h"

#define INVALID_INDEX 11
#define INVALID_BOUNDARY 2

/* Generate a Huffman Tree from a given set of values and frequencies */
void generateHuffman(uint16_t *freq);

/* Generate a bit stream from a given Huffman Tree */
void generateStream(uint16_t *word, uint16_t *stream, uint16_t *timeWords);

/* Generate the signal times from a given bit stream */
void generateTimes(uint16_t *bitStream, uint16_t *word, uint16_t *freq, uint16_t *timeWords);

/* Checks the validity of a boundary case Huffman code being read back */
uint16_t check_boundary_code(uint16_t currentStream, uint16_t nextStream, uint16_t startBit);

/* Returns the desired bit from a word */
uint16_t get_bit(uint16_t value, uint16_t i);
