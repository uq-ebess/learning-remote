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
 * defines.h
 *
 * Created: 13/07/2015 11:52:00
 *  Author: Marcus Herbert
 */ 

#define BAUD_RATE 19200
#define F_CPU 8000000UL

#define NUMBER_OF_PULSES 170
#define DIFFERENCE 10
#define UNIQUE_VALUES 6

#define INVALID_BUTTON 13
#define INVALID_SIGNAL 14

#define TIME_ALLOCATED_WORDS UNIQUE_VALUES
#define HUFFMAN_ALLOCATED_WORDS 16

#define WORDS_OF_TIME NUMBER_OF_PULSES

#define HUFFMAN_THRESHOLD 12000
