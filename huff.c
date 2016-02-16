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
 * huff.c
 *
 * Created: 1/07/2015 22:58:00
 *  Author: Marcus Herbert
 */ 

//#include <stdio.h>
//#include <stdlib.h>
#include <avr/io.h>
#include "remoteControl.h"
#include "huff.h"

/* Accounted for */
//uint16_t freq[TIME_ALLOCATED_WORDS] = {1, 3, 102, 42, 3, 0, 0, 0, 0, 0};
/* Accounted for */
//uint16_t word[TIME_ALLOCATED_WORDS] = {47807, 222, 48, 160, 9830, 0, 0, 0, 0, 0};
/* Accounted for */
//uint16_t timeWords[WORDS_OF_TIME];
/* Accounted for */
//uint16_t stream[HUFFMAN_ALLOCATED_WORDS]; //Make sure to set this to 0 initially. Individual word are {LastVal.....FirstVal} (LSbit to MSbit)

/* These are the remaining globals */
uint16_t huffCodes[TIME_ALLOCATED_WORDS];
uint16_t bitsToConsider[TIME_ALLOCATED_WORDS];
uint16_t maximum;

/* Returns the desired bit from a word */
uint16_t get_bit(uint16_t value, uint16_t i) {
    uint16_t bit = !!(value & (1 << i));
    return bit;
}

/* Checks the validity of a boundary case Huffman code being read back */
uint16_t check_boundary_code(uint16_t currentStream, uint16_t nextStream, uint16_t startBit) {
    //Might need to handle inserting nextStream when in the last stream element (reading outside memory range)
    uint16_t bitsLeft = startBit + 1;
    uint16_t insideBoundary;
    uint16_t combinedStream = 0;
    uint16_t index = INVALID_INDEX;
	
    if (bitsLeft >= maximum) { //No need to check, impossible to overlap
		//print_uint16_t(25); next_line();
        insideBoundary = 1;
    } else { //Potential for overlap between stream elements, so we check
		//print_uint16_t(22); next_line();
		for (int r = 0; r < bitsLeft; r++) { //Put the last remaining currentStream bits into combinedStream
			combinedStream |= (get_bit(currentStream, r) << (15 + r - startBit));
		}
		/*print_uint16_t(23); next_line();
		print_string("maximum: "); print_uint16_t(maximum); next_line();
		print_string("bitsLeft: "); print_uint16_t(bitsLeft); next_line();*/
		for (int r = 0; r < (maximum - bitsLeft); r++) { //Now put the first bits from nextStream into combinedStream
			combinedStream |= (get_bit(nextStream, 15 - r) << (15 - r - bitsLeft));
			//print_uint16_t(get_bit(nextStream, 15 - r)); next_line(); print_uint16_t((15 - r - bitsLeft)); next_line();
		}
		//print_uint16_t(24); next_line();
		
		//print_uint16_t(26); next_line();
        for (int c = 0; c < TIME_ALLOCATED_WORDS; c++) {
            //if (!originalFreq[c]) continue; //Ignore if zero frequency (this may be unnecessary
            for (int m = 0; m < bitsToConsider[c]; m++) { //Iterate over the valid Huffman code only as far as is legal
				//print_uint16_t(27); next_line();
                if (get_bit(combinedStream, 15 - m) == get_bit(huffCodes[c], maximum - m - 1)) { // The bit matches a Huffman code bit
                    index = c;
					//print_uint16_t(28); next_line();
                } else { //This Huffman code ended up being invalid
					//print_uint16_t(29); next_line();
                    index = INVALID_INDEX; //Reset time index as this isn't the valid Huffman code
                    break;
                }
            }

            if (index != INVALID_INDEX) break;
        }
        if (bitsToConsider[index] > bitsLeft) {
            insideBoundary = 0; //The code must overlap
        } else {
            insideBoundary = 1; //The code must fit
        }            
    }

    return insideBoundary;
}

/* Generate the signal times from a given bit stream */
void generateTimes(uint16_t *bitStream, uint16_t *word, uint16_t *freq, uint16_t *timeWords) {
    uint16_t timeIndex = INVALID_INDEX; //11 Indicates an invalid index
    uint16_t counter = 0;
    uint16_t boundary = 0;
    uint16_t insideBoundary = INVALID_BOUNDARY;
    uint16_t bitsLeft = 0;
    //uint16_t save = 0; //Debugging
    uint16_t totalFreq = 0;

    for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) totalFreq += freq[i];
	
	//totalFreq = 2;

    for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) { //Iterate over the stream words
        for (int j = 15; j >= 0; j--) { //Iterate over the bits of a stream word
            for (int k = 0; k < TIME_ALLOCATED_WORDS; k++) { //Iterate over valid time words get_bit(huffCodes[k], maximum)
                if (counter >= (totalFreq)) break;
                if (!insideBoundary) { //We have a boundary case left over from last time that we need to inspect
                    uint16_t tempStream = boundary;

                    /* Not sure if to use j or 15 here (in the boundary's get_bit), as I think we should only get here at the start of a new iteration */
                    if (get_bit(boundary, 15) == get_bit(huffCodes[k], maximum - 1)) { //We've found a valid (non-unique) Huffman code 
                        for (int w = 15; w >= (15 - (bitsToConsider[k] - bitsLeft - 1)); w--) {
                            tempStream |= (get_bit(bitStream[i], w) << (w - bitsLeft));
                        }

                        for (int m = 0; m < bitsToConsider[k]; m++) { //Iterate over the valid Huffman code only as far as is legal
                            if (get_bit(tempStream, j - m) == get_bit(huffCodes[k], maximum - m - 1)) { // The bit matches a Huffman code bit
                                timeIndex = k;

                            } else { //This Huffman code ended up being invalid
                                timeIndex = INVALID_INDEX; //Reset time index as this isn't the valid Huffman code
                                break;
                            }
                        }
                        
                        // Remember to alter j by the appropriate amount when the correct code has been found here
                        // Remember to reset insideBoundary here when done if correct code
                        // Reset bitsLeft here if correct code (not needed anymore), reset later on

                        if (timeIndex != INVALID_INDEX) { //We're good to go!
                            j = j - (bitsToConsider[timeIndex] - bitsLeft - 1); //-1 to account for zero indexing
                            //printf("%d, ", word[timeIndex]);
                            timeWords[counter++] = word[timeIndex];

                            timeIndex = INVALID_INDEX;
                            insideBoundary = INVALID_BOUNDARY;

                            break;
                        }
                    }
                } else { //No boundary case left over, continue as per normal
                    if (get_bit(bitStream[i], j) == get_bit(huffCodes[k], maximum - 1)) { //We've found a valid (non-unique) Huffman code 
                        insideBoundary = check_boundary_code(bitStream[i], bitStream[i + 1], j);

						

						/*print_string("Here is the bit stream:\r\n");
						print_binary(bitStream[i], 15); next_line();
						print_string("Here are the huffCodes:\r\n");
						for (int t = 0; t < TIME_ALLOCATED_WORDS; t++) {
							print_binary(huffCodes[t], 3); 
							usart_transmit('-');
						}
						print_string("\r\n");*/

                        boundary = 0;
                        bitsLeft = 0;
                        if (insideBoundary) {
                            for (int m = 0; m < bitsToConsider[k]; m++) { //Iterate over the valid Huffman code only as far as is legal
								/*print_string("j: "); print_uint16_t(j); next_line();
								print_string("m: "); usart_transmit((unsigned char) '0' + m); next_line();
								print_string("maximum: "); usart_transmit((unsigned char) '0' + maximum); next_line();
								usart_transmit((unsigned char) '0' + get_bit(bitStream[i], j - m)); usart_transmit('=');
								usart_transmit((unsigned char) '0' + get_bit(huffCodes[k], maximum - m - 1)); next_line();
								print_string("k: "); usart_transmit((unsigned char) '0'+ k); next_line();
								print_string("timeIndex: "); usart_transmit((unsigned char) '0'+ timeIndex); next_line();
								print_binary(huffCodes[k], 3); line_break();*/
                                if (get_bit(bitStream[i], j - m) == get_bit(huffCodes[k], maximum - m - 1)) { // The bit matches a Huffman code bit
                                    timeIndex = k;
									//print_string("made it\r\n");
                                } else { //This Huffman code ended up being invalid
									//print_string("Forget about me\r\n");
                                    timeIndex = INVALID_INDEX; //Reset time index as this isn't the valid Huffman code
                                    break;
                                }
                            }
                        } else { //We need to see how many bits are left in the current word
                            bitsLeft = j + 1; //Save how many bits are left in the current word
                            boundary = 0; //Reset the variable
                            for (int n = 0; n < bitsLeft; n++) {
                                boundary |= get_bit(bitStream[i], j - n) << (15 - n); //Save whatever stream bits are left over 
                            }
                            j = 0;
                            break;
                        }
                    }
                }
                if (timeIndex != INVALID_INDEX) { //Matching code found, so we stop searching
                    //printf("%d, ", word[timeIndex]);
                    timeWords[counter++] = word[timeIndex];
                    //counter++;
                    
                    //printf("counter: %d, totalFreq: %d\n", counter, totalFreq);
                    break;
                }
            }
            if (counter > (totalFreq)) {
                //printf("\n");
                break;
            }

            if (timeIndex != INVALID_INDEX) {
                j = j + 1 - bitsToConsider[timeIndex];
                timeIndex = INVALID_INDEX;
            }
        }
        if (counter > (totalFreq)) {
            //printf("\n");
            break;
        }
    }
}

/* Generate a bit stream from a given Huffman Tree */
void generateStream(uint16_t *word, uint16_t *stream, uint16_t *timeWords) {
    uint16_t temp;
    uint16_t bitsLeft = 16; //To count how many bits left in a word of the stream
    uint16_t index = 0;
    uint16_t maxBits = 0;
    uint8_t wordDone = 0; //Use this to reset the bitsLeft counter
    uint16_t limit = 0; //Number of bits that can fit in the current word

    for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
        if (bitsToConsider[i] > maxBits) maxBits = bitsToConsider[i];
    }
    
    maximum = maxBits;

    for (int i = 0; i < WORDS_OF_TIME; i++) {
        temp = timeWords[i]; 
        uint16_t test;
        if (!temp) continue; //Skip if there's a zero in the array
        for (int j = 0; j < TIME_ALLOCATED_WORDS; j++) {
            test = bitsLeft - bitsToConsider[j]; //This is to test if we have enough space left in the current word for the Huffman bits
            if ((temp >= (word[j] - 10)) && (temp <= (word[j] + 10))) { //We've found the corresponding time value
                if (((int16_t) test) >= 0) { //There are enough bits remaining in the word for the needed Huffman code
                    for (int k = 0; k < bitsToConsider[j]; k++) {
                        stream[index] |= (!!(huffCodes[j] & (1 << (maxBits - k - 1)))) << (bitsLeft - 1 - k); //We subtract 1 to account for zero-indexing
                    }
                    bitsLeft -= bitsToConsider[j];
  
                } else { //There are not enough bits left in the current word for the entire needed Huffman code
                    for (int q = 0; q < maxBits; q++) {
                        if (((int16_t) test) == q + -1*((int16_t) bitsToConsider[j])) { //Check how many bits can fit in the current word
                            limit = q; //Save the amount of bits that can fit
                            wordDone = 1;
                        }
                    }
                }

                if (wordDone) {
                    for (int k = 0; k < (limit); k++) { //Append the bits that can be to the end of the current word
                        stream[index] |= (!!(huffCodes[j] & (1 << (maxBits - k - 1)))) << (bitsLeft - 1 - k); //We subtract 1 to account for zero-indexing
                    }
                    
                    index++; //Move to the next word in the stream
                    bitsLeft = 16; 

                    for (int k = (limit); k < bitsToConsider[j]; k++) { //Stick the remaining bits at the start of the next word
                        stream[index] |= (!!(huffCodes[j] & (1 << (maxBits - k - 1)))) << (bitsLeft - 1 - k + limit); //We subtract 1 to account for zero-indexing
                        /* The + limit in the shift at the end resets our initial shift offset back to 0.
                         * This is a special case because our for loop doesn't start at 0, so we have to move the initial offset back to 0
                         */
                    }

                    wordDone = 0; //Reset the flag
                    bitsLeft -= (bitsToConsider[j] - limit); //Set the number of bits left after adding the extra bits to the start of the new word
                }
                break; //Stop iterating over the for loop with j, as we found our word   
            }
        }
    }
}

/* Generate a Huffman Tree from a given set of values and frequencies */
void generateHuffman(uint16_t *freq) {
	uint16_t levels[TIME_ALLOCATED_WORDS];
	char leftOrRight[TIME_ALLOCATED_WORDS];
	for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
		levels[i] = 0;
		leftOrRight[i] = 0;
		huffCodes[i] = 0; //Reset the globals here
		maximum = 0;
		bitsToConsider[i] = 0;
	}
	
	uint16_t branchIDs[TIME_ALLOCATED_WORDS]; // = {0, 1, 2, 3, 4}; //, 5, 6, 7}; //Arbitrary to make them all different at the start
	for (int d = 0; d < TIME_ALLOCATED_WORDS; d++) branchIDs[d] = d;
    uint16_t freqCounter = 7; //Arbitrary non-one initial value
    uint16_t ID = 10;

    while (freqCounter != 1) {
        ID++;
        uint16_t lowest = 0xFFFF, secondLowest = 0xFFFF;
        uint16_t indices[2] = {INVALID_INDEX, INVALID_INDEX};

        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if (!freq[i]) continue;

            if (freq[i] < lowest) {
                lowest = freq[i];
                indices[0] = i;
            }
        }

        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if (!freq[i]) continue;

            if (i == indices[0]) continue; //Skip if we already used this index in the first loop
            if (freq[i] < secondLowest) {
                secondLowest = freq[i];
                indices[1] = i;
            }
        }

        freq[indices[0]] += freq[indices[1]]; //Add the two frequencies together
        freq[indices[1]] = 0; //Clear this for future rounds
       
        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if ((i == indices[0]) || (i == indices[1])) continue; //So as not to overwrite previous branchIDs

            if (branchIDs[i] == branchIDs[indices[0]]) {
                branchIDs[i] = ID;
                leftOrRight[i] = 'L';
            }

            if(branchIDs[i] == branchIDs[indices[1]]) {
                branchIDs[i] = ID;
                leftOrRight[i] = 'R';
            }
        }

        leftOrRight[indices[0]] = 'L'; leftOrRight[indices[1]] = 'R';
        branchIDs[indices[0]] = branchIDs[indices[1]] = ID; //Given current leaves current branch ID

        if (levels[indices[0]] > levels[indices[1]])        levels[indices[1]] = levels[indices[0]]; //Equalise levels
        else if (levels[indices[1]] > levels[indices[0]])   levels[indices[0]] = levels[indices[1]];

        levels[indices[0]]++; levels[indices[1]]++; 


        uint16_t highestLevel = 0;
        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if ((i == indices[0]) || (i == indices[1])) continue; //So as not to overwrite previous levels

            if (branchIDs[i] == ID) {
                if (levels[i] > highestLevel) highestLevel = levels[i]; //Find the highest level for the current branch
            }
        }

        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if (branchIDs[i] == ID) {
                if (leftOrRight[i] == 'L')      huffCodes[i] |= (0 << highestLevel);
                else if (leftOrRight[i] == 'R') huffCodes[i] |= (1 << highestLevel);
                bitsToConsider[i]++;
            }
        }

        freqCounter = 0;
        for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
            if (freq[i]) {
                freqCounter++;
            }
        }

    }
	
	for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) {
		if (bitsToConsider[i] > maximum) maximum = bitsToConsider[i];
	}
}

/*
int main (int argc, char **argv) {
    //for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) originalFreq[i] = freq[i];
    ///////////////////////////////////
    generateHuffman(frequencies);
    ///////////////////////////////////

    /////////////////////////
    generateStream(words, stream, timeWords);
    /////////////////////////
	
	// words = array (10 word) of unique time lengths within boundaries, sorted for Huffman
	// stream = a 15 word array to be filled with the bit stream
	// timeWords = raw highValues or lowValues (170 or 190 or so elements)
	// frequencies = 10 word array of summed frequencies of unique time length in words
	 

    ////////////////////////////
    generateTimes(stream, words, frequencies);
    ////////////////////////////

    return 0;
}
*/
