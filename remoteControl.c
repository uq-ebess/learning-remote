/*
 * remoteControl.c
 *
 * Created: 20/06/2015 15:33:41
 *  Author: Marcus Herbert
 */ 

#include "remoteControl.h"
#include "huff.h"
#include "eepromAux.h"

#include <avr/sleep.h>
#include <util/delay.h>

//Uncomment this for debug printing (slows down program)
//#define DEBUG_PRINT
//#define EXTRA_DEBUG_PRINT
//#define RX_DEBUGGING
//#define LAST_DEBUGGING

//#define BUTTON_PRESSED 0
//These three are indices for the state variable
#define MODE 0	
#define TX_FLAG 1
//#define TX_BUTTON_ID_SHIFT 2

//These values determine which mode the remote is in
#define TX_MODE 0
#define RX_MODE 1

//This indicates whether we want to transmit or not
#define TRANSMIT 1

//The mode toggle button is on B0, so this checks whether it is pressed or not
#define CHECK_TOGGLE() (PINB & (1 << PINB0))
#define TOGGLE_MODE() (state ^= (1 << MODE))

//These are only used with the state variable and EIMSK to avoid any size/type issues
#define RETRIEVE_BIT(BYTE, INDEX)	!!(BYTE & (1 << INDEX))
#define SET_BIT(BYTE, INDEX)		(BYTE |= (1<<INDEX)) //Use with |=
#define CLEAR_BIT(BYTE, INDEX)		(BYTE &= ~(1 << INDEX)) //Use with &=

#define CLEAR_INT1_FLAG()	(SET_BIT(EIFR, INTF1))
#define ENABLE_INT1()		(SET_BIT(EIMSK, INT1))
#define DISABLE_INT1()	(CLEAR_BIT(EIMSK, INT1))

#define CLEAR_INT0_FLAG() (SET_BIT(EIFR, INTF0))

#define RX_TIMEOUT 610 //20 seconds [register = (time/((256*1024)/8000000))]

#define RESET_WATCHDOG() __asm__ __volatile__ ("wdr")

//The almighty state variable
volatile uint8_t state = 0;

volatile uint8_t startFlag;
uint8_t pcint11Counter;

volatile uint16_t highTime[NUMBER_OF_PULSES];
volatile uint16_t lowTime[NUMBER_OF_PULSES];

uint16_t lowValues[UNIQUE_VALUES];
uint16_t highValues[UNIQUE_VALUES];

uint16_t lowOccuranceCounter[UNIQUE_VALUES];
uint16_t highOccuranceCounter[UNIQUE_VALUES];

volatile int lowCount;
volatile int highCount;

uint8_t lastTxSignal = INVALID_SIGNAL;
volatile uint8_t txButton = INVALID_BUTTON; //Start on an invalid button
volatile uint8_t rxButton = INVALID_BUTTON;

volatile uint16_t rxTicks;
volatile uint16_t breakout;

/* Initialise the timer0 interrupt */
void init_timer0(void) {
	TIMSK0 = (1 << OCIE0A);
}

/* Starts timer 0 at a prescale of 1024 */
void start_timer0(void) {
	TCNT0;
	TCCR0B = (1 << CS02) | (1 << CS00);
}

/* Stops timer 0 */
void stop_timer0(void) {
	TCCR0B = 0;
}

/* Prints out a string */
void print_string(char *string) {
	for (int b = 0; string[b]; b++) usart_transmit((unsigned char) string[b]);
}

/* Initialises the watchdog timer */
void init_watchdog_timer(void) {
	cli();
	RESET_WATCHDOG();
	
	MCUSR &= ~(1 << WDRF); //Clear the flag
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0); //System reset mode, 0.5s reset
	sei();
}

/* Resets and turns off the watchdog timer */
void turn_off_WDT(void) {
	cli();
	RESET_WATCHDOG();
	
	MCUSR &= ~(1 << WDRF);
	WDTCSR |= (1 << WDCE) | (1 << WDE); //Must set WDCE in order to clear WDE
	WDTCSR = 0; //Clear WDE and everything else
	
	sei();
}

/* Prints the binary representation of a uint16_t number from a certain start bit */
void print_binary(uint16_t num, uint16_t startBit) {
	for (int i = startBit; i >= 0; i--) {
		unsigned char byto = !!(num & (1<<i)) + 48;
		usart_transmit(byto);
	}
}

void check_rx_buttons(void) {
	//Check PINC (1-5)
	rxButton = INVALID_BUTTON;
	uint8_t variableShift = 0;
	
	for (uint8_t z = 1; z <= 5; z++) {
		if (PINC & (1 << z)) rxButton = variableShift;
		variableShift++;
	}
	
	//Check PIND (5-7)
	for (uint8_t z = 5; z <= 7; z++) {
		if (PIND & (1 << z)) rxButton = variableShift;
		variableShift++;
	}
	
	//Check PINB (6-7)
	for (uint8_t z = 6; z <= 7; z++) {
		if (PINB & (1 << z)) rxButton = variableShift;
		variableShift++;
	}
}

int main(void) {
	init_hardware();
	//Set the first transmit button to be invalid
	txButton = INVALID_BUTTON;
	//Set the initially 'saved' signal to be invalid
	lastTxSignal = INVALID_SIGNAL;
	//SET_BIT(PORTB, PINB1);
	//for (long a = 1; a > 0; a++);
	//CLEAR_BIT(PORTB, PINB1);
	RESET_WATCHDOG();
    while(1) {
		#ifdef EXTRA_DEBUG_PRINT
		print_string("START: Well, I made it to here\r\n");
		#endif
		if (RETRIEVE_BIT(state, MODE) == TX_MODE) { //TX mode
			#ifdef EXTRA_DEBUG_PRINT
			print_string("In TX mode\r\n");
			#endif
			RESET_WATCHDOG();
			cli();
			//Debouncing
			_delay_ms(5);
			rxButton = INVALID_BUTTON;
			check_rx_buttons();
			
			#ifdef LAST_DEBUGGING
			print_string("In TX mode\r\n");
			print_uint16_t((uint16_t) rxButton); next_line();
			print_uint16_t((uint16_t) txButton); next_line();
			#endif
			
			#ifdef LAST_DEBUGGING
			print_string("1\r\n");
			#endif
			RESET_WATCHDOG();
			if (rxButton < INVALID_BUTTON) {
				#ifdef LAST_DEBUGGING
				print_string("2\r\n");
				#endif
				//Check whether we are to transmit
				if (RETRIEVE_BIT(state, TX_FLAG) == TRANSMIT) {
					#ifdef LAST_DEBUGGING
					print_string("3\r\n");
					#endif
					DISABLE_INT1();
					//Initialise TX mode
					cli();
					CLEAR_BIT(state, TX_FLAG); //Clear the bit declaring that we want to transmit
					sei();
					RESET_WATCHDOG();
					
					#ifdef LAST_DEBUGGING
					cli();
					print_string("txButton: "); print_uint16_t((uint16_t) txButton); next_line();
					print_string("lastTxSignal: "); print_uint16_t((uint16_t) lastTxSignal); next_line();
					
					if (txButton == lastTxSignal) {
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) highTime); next_line();
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) lowTime); next_line();
					}
					sei();
					#endif
					RESET_WATCHDOG();
					//Check if data needs to be retrieved from EEPROM or not
					if ((txButton != lastTxSignal) && (txButton < INVALID_BUTTON)) { //We don't have the data on hand
						#ifdef LAST_DEBUGGING
						print_string("e4\r\n");
						#endif
						cli();
						//print_string("Sending new stuff\r\n");
						//Clear highTime, lowTime, lowValues and highValues
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++)	highValues[i]	= 0;
						for (int i = 0; i < NUMBER_OF_PULSES; i++)		highTime[i]		= 0;
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++)	lowValues[i]	= 0;
						for (int i = 0; i < NUMBER_OF_PULSES; i++)		lowTime[i]		= 0;
						RESET_WATCHDOG();
						//This lets us know what signal is currently stored in the global variable arrays
						lastTxSignal = txButton;
						uint8_t freqz[TIME_ALLOCATED_WORDS];
						uint16_t huffmanStream[HUFFMAN_ALLOCATED_WORDS];
						
						for (int s = 0; s < TIME_ALLOCATED_WORDS; s++) lowOccuranceCounter[s] = highOccuranceCounter[s] = 0;
						
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) freqz[i] = 0;
						for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) huffmanStream[i] = 0;
						
						RESET_WATCHDOG();
						#ifdef DEBUG_PRINT
						usart_transmit('p'); usart_transmit('r'); line_break();
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) lowTime);
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) highTime);
						#endif

						//Retrieve high signal
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) freqz[i] = 0;
						for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) huffmanStream[i] = 0;
						RESET_WATCHDOG();
						eeprom_retrieve_high_signal(highValues, huffmanStream, freqz, lastTxSignal);
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) highOccuranceCounter[i] = (uint16_t) freqz[i];
						generateHuffman(highOccuranceCounter); RESET_WATCHDOG();
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) highOccuranceCounter[i] = (uint16_t) freqz[i];
						generateTimes(huffmanStream, highValues, highOccuranceCounter, (uint16_t *) highTime); RESET_WATCHDOG();
						
						
						//Retrieve low signal
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) freqz[i] = 0;
						for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) huffmanStream[i] = 0;
						
						eeprom_retrieve_low_signal(lowValues, huffmanStream, freqz, lastTxSignal);
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) lowOccuranceCounter[i] = (uint16_t) freqz[i];
						generateHuffman(lowOccuranceCounter); RESET_WATCHDOG();
						for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) lowOccuranceCounter[i] = (uint16_t) freqz[i];
						generateTimes(huffmanStream, lowValues, lowOccuranceCounter, (uint16_t *) lowTime); RESET_WATCHDOG();
						

						#ifdef DEBUG_PRINT
						usart_transmit('d'); usart_transmit('o'); line_break();
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) lowTime);
						print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) highTime);
						#endif
						//print_string("Done decompressing\r\n");
						sei();
						#ifdef LAST_DEBUGGING
						print_string("e5\r\n");
						#endif
					}
					#ifdef LAST_DEBUGGING
					print_string("6\r\n");
					#endif
					//CLEAR_BIT(state, TX_FLAG);
					turn_off_WDT();
					send_data();
					init_watchdog_timer();
					CLEAR_INT1_FLAG();
					ENABLE_INT1();
					#ifdef LAST_DEBUGGING
					print_string("7\r\n");
					#endif
				}
				#ifdef LAST_DEBUGGING
				print_string("8\r\n");
				#endif
			
			}
			#ifdef LAST_DEBUGGING
			print_string("9\r\n");
			#endif
			
		} else { //RX mode
			#ifdef RX_DEBUGGING
			print_string("1\r\n");
			#endif
			RESET_WATCHDOG();
			//Initialise RX state
			lastTxSignal = INVALID_SIGNAL;
			breakout = 0;
			rxTicks = 0;
			start_timer0();
			DISABLE_INT1(); //Be careful here
			CLEAR_INT1_FLAG();
			RESET_WATCHDOG();
			#ifdef EXTRA_DEBUG_PRINT
			print_string("In RX mode\r\n");
			#endif
			
			DDRB |= (1 << PINB1);
			PORTB |= (1 << PINB1); //Turn on RX LED
			cli();
			#ifdef RX_DEBUGGING
			print_string("2\r\n");
			#endif
			RESET_WATCHDOG();
			startFlag = 0;
			TIFR1 |= (1<<TOV1);
			//Clear highTime, lowTime, lowValues and highValues
			for (int i = 0; i < TIME_ALLOCATED_WORDS; i++)	highValues[i]	= lowValues[i]	= 0;
			RESET_WATCHDOG();
			for (int i = 0; i < NUMBER_OF_PULSES; i++)		highTime[i]		= lowTime[i]	= 0;
			//for (int i = 0; i < TIME_ALLOCATED_WORDS; i++)	lowValues[i] = 0;
			//for (int i = 0; i < NUMBER_OF_PULSES; i++)		lowTime[i] = 0;
			
			rxButton = INVALID_BUTTON;
			DDRD |= (1<<DDD4); //Put recording transistor to output
			PORTD |= (1<<PD4); //Turn on the recording transistor
			
			RESET_WATCHDOG();
			_delay_ms(20);
			while(CHECK_TOGGLE()) RESET_WATCHDOG();
			_delay_ms(20);
			RESET_WATCHDOG();
			#ifdef RX_DEBUGGING
			print_string("3\r\n");
			#endif
			
			#ifdef DEBUG_PRINT
			while(1) {
				print_string("Toggle button: "); print_uint16_t((uint16_t) CHECK_TOGGLE()); next_line();
			}
			#endif
			CLEAR_INT0_FLAG();
			CLEAR_INT1_FLAG();
			
			#ifdef RX_DEBUGGING
			print_string("4\r\n");
			#endif
			sei();
			while ((rxButton >= INVALID_BUTTON) & (!breakout)) {
				RESET_WATCHDOG();
				check_rx_buttons();
				_delay_ms(20);
				check_rx_buttons();
				if (CHECK_TOGGLE()) break;
			}
			RESET_WATCHDOG();
			#ifdef RX_DEBUGGING
			print_string("5\r\n");
			print_string("rxButton: "); print_uint16_t((uint16_t) rxButton); next_line();
			#endif
			#ifdef EXTRA_DEBUG_PRINT
			print_string("rxButton: "); print_uint16_t((uint16_t) rxButton); next_line();
			#endif
			
			if (rxButton < INVALID_BUTTON) {
				cli();
				#ifdef DEBUG_PRINT
				print_string("rxButton: "); print_uint16_t(rxButton); next_line();
				#endif
				#ifdef RX_DEBUGGING
				print_string("6\r\n");
				#endif
				DDRB |= (1 << PINB2);
				PORTB |= (1 << PORTB2); //Turn on green LED
					
				//_delay_ms(500);
				//CLEAR_INT0_FLAG();
				sei();
				enable_int0();
				RESET_WATCHDOG();
				//uint8_t breakout = 0;
				while(!receive_complete()) {
					RESET_WATCHDOG();
					//print_string("Here\r\n");
					if (CHECK_TOGGLE() | breakout) {
						breakout++;
						break;
					}
				}
				#ifdef RX_DEBUGGING
				print_string("7\r\n");
				#endif
				if (!breakout) {
					cli();
					#ifdef DEBUG_PRINT
					print_string("Receiving stuff\r\nReceived:\r\n");
					//usart_transmit('d'); usart_transmit('o'); line_break();
					print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) lowTime);
					print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) highTime);
					#endif 
					RESET_WATCHDOG();
					//print_string("Inside receive complete\r\n");
					uint8_t freqz[TIME_ALLOCATED_WORDS];
					uint16_t huffmanStream[HUFFMAN_ALLOCATED_WORDS];
					
					////////////////////////////// Calculate high signal //////////////////////////////
					for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) freqz[i] = (uint8_t) highOccuranceCounter[i];
					for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) huffmanStream[i] = 0;
					RESET_WATCHDOG();
					generateHuffman(highOccuranceCounter); RESET_WATCHDOG();
					generateStream(highValues, huffmanStream, (uint16_t *) highTime); RESET_WATCHDOG();
					eeprom_save_high_signal(highValues, huffmanStream, freqz, rxButton); RESET_WATCHDOG();
					
					for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) highOccuranceCounter[i] = (uint16_t) freqz[i];
					generateTimes(huffmanStream, highValues, highOccuranceCounter, (uint16_t *) highTime); RESET_WATCHDOG();
					
					#ifdef DEBUG_PRINT
					print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) highTime);
					#endif
					
					////////////////////////////// Calculate low signal //////////////////////////////
					for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) freqz[i] = (uint8_t) lowOccuranceCounter[i];
					for (int i = 0; i < HUFFMAN_ALLOCATED_WORDS; i++) huffmanStream[i] = 0; //I might not need this

					generateHuffman(lowOccuranceCounter); RESET_WATCHDOG();
					generateStream(lowValues, huffmanStream, (uint16_t *) lowTime); RESET_WATCHDOG();
					eeprom_save_low_signal(lowValues, huffmanStream, freqz, rxButton); RESET_WATCHDOG();

					for (int i = 0; i < TIME_ALLOCATED_WORDS; i++) lowOccuranceCounter[i] = (uint16_t) freqz[i];
					generateTimes(huffmanStream, lowValues, lowOccuranceCounter, (uint16_t *) lowTime); RESET_WATCHDOG();
				
					#ifdef DEBUG_PRINT
					print_uint16_t_array(0, NUMBER_OF_PULSES, (uint16_t *) lowTime);
					#endif
				}
				//rxButton = INVALID_BUTTON;
				//rxCounter = 0;
				
				sei();
				disable_int0();
				RESET_WATCHDOG();
				//}
			}
			if (TCCR0B) stop_timer0();
			
			PORTB &= ~(1<<PORTB2); //Turn off button select LED
			PORTB &= ~(1<<PORTB1);
			DDRB &= ~((1 << PINB1) | (1 << PINB2));
			
			_delay_ms(1000); //This should force a system reset
		}
		RESET_WATCHDOG();
		#ifdef LAST_DEBUGGING
		print_string("10\r\n");
		#endif
		rxButton = INVALID_BUTTON;
		txButton = INVALID_BUTTON;
		
		PORTD &= ~(1<<PD4); //Zero the output to the recording transistor
		DDRD &= ~(1<<DDD4); //Put recording transistor to an input (basically only for power saving)
		CLEAR_INT1_FLAG();
		ENABLE_INT1();
		
		disable_int0();
		CLEAR_INT0_FLAG();
		
		CLEAR_BIT(state, MODE); //Force us to TX
		PORTB &= ~((1 << PORTB2) | (1 << PORTB1)); //Turn off RX LEDs
		DDRB &= ~((1 << PINB2) | (1 << PINB1));
		
		#ifdef EXTRA_DEBUG_PRINT
		print_string("Gone to sleep\r\n");
		#endif
		
		#ifdef LAST_DEBUGGING
		//print_string("Before toggle check\r\n");
		print_string("11\r\n");
		#endif
		RESET_WATCHDOG();
		if (CHECK_TOGGLE()) {
			cli();
			while (CHECK_TOGGLE()) RESET_WATCHDOG();
			_delay_ms(10);
			sei();
		}
		
		#ifdef LAST_DEBUGGING
		//print_string("After toggle check\r\n");
		print_string("12\r\n");
		#endif
		
		turn_off_WDT();
		sleep();
		init_watchdog_timer();
		
		//_delay_ms(100);
		//CLEAR_INT1_FLAG();
		//DISABLE_INT1();
		#ifdef LAST_DEBUGGING
		print_string("0\r\n");
		#endif
		
		#ifdef EXTRA_DEBUG_PRINT
		print_string("Woken up\r\n");
		/*print_string("Mode: "); print_uint16_t((uint16_t) RETRIEVE_BIT(state, MODE)); next_line();
		print_string("Transmit: "); print_uint16_t((uint16_t) RETRIEVE_BIT(state, TX_FLAG)); next_line();
		print_string("txButton: "); print_uint16_t((uint16_t) txButton); next_line();
		print_string("rxButton: "); print_uint16_t((uint16_t) rxButton); next_line();*/
		#endif
		//ENABLE_INT1();
		//_delay_ms(1000);
		//DISABLE_INT1();
	}
}

/* Prints out a single array from start to finish */
void print_uint16_t_array(uint16_t start, uint16_t finish, uint16_t *array) {
	for (int i = start; i < finish; i++) {
		print_uint16_t(array[i]);
		usart_transmit('-');
	}
	line_break();
}

/* Prints out an unsigned 16 bit integer */
void print_uint16_t(uint16_t number) {
	uint16_t powerOfTen = 10000;
	for (int j = 4; j >= 0; j--) {
		usart_transmit((unsigned char) 48 + ((number / powerOfTen) % 10));
		powerOfTen /= 10;
	}
}

/* Moves to the start of the next line */
void next_line(void) {
	usart_transmit('\n');
	usart_transmit('\r');
}

/* Prints a spaced line */
void line_break(void) {
	usart_transmit('\n');
	usart_transmit('\r');
	usart_transmit('\n');
}

/* Sorts the received data */
void array_sorter(void) {
	uint8_t lowArrayCounter = 0;
	uint8_t highArrayCounter = 0;
	uint8_t invalidCounter = 0;
	for (int i = 0; i < UNIQUE_VALUES; i++) lowOccuranceCounter[i] = highOccuranceCounter[i] = 0;
	
	for (int i = 0; i < NUMBER_OF_PULSES; i++){
		if (lowTime[i]) {
			for (int j = 0; j < UNIQUE_VALUES; j++) {
				if (!(((lowTime[i] + DIFFERENCE) > lowValues[j]) && ((lowTime[i] - DIFFERENCE) < lowValues[j]))) {
					invalidCounter++; //The current time is not in the compared stored value's range
				} else { //The current time is within a compared stored value's range, thus we have already saved it and so we ignore and break
					invalidCounter = 0;
					lowOccuranceCounter[j]++;
					break;
				}
			}
			if (invalidCounter) { //This happens the first time we encounter a unique time range
				lowOccuranceCounter[lowArrayCounter]++;
				lowValues[lowArrayCounter++] = lowTime[i];
				}
		}
		
	}
	
	invalidCounter = 0;
	for (int i = 0; i < NUMBER_OF_PULSES; i++){
		if (highTime[i]) {
			for (int j = 0; j < UNIQUE_VALUES; j++) {
				if (!(((highTime[i] + DIFFERENCE) > highValues[j]) && ((highTime[i] - DIFFERENCE) < highValues[j]))) {
					invalidCounter++; //The current time is not in the compared stored value's range
				} else { //The current time is within a compared stored value's range, thus we have already saved it and so we ignore and break
					invalidCounter = 0;
					highOccuranceCounter[j]++;
					break;
				}
			}
			if (invalidCounter) {
				highOccuranceCounter[highArrayCounter]++;
				highValues[highArrayCounter++] = highTime[i];
			}
		}
	}
	
	#ifdef DEBUG_PRINT
	usart_transmit('L');
	usart_transmit('V');
	next_line();
	print_uint16_t_array(0, UNIQUE_VALUES, lowValues);
	
	print_uint16_t_array(0, UNIQUE_VALUES, lowOccuranceCounter);
	
	usart_transmit('H');
	usart_transmit('V');
	next_line();
	print_uint16_t_array(0, UNIQUE_VALUES, highValues);
	
	print_uint16_t_array(0, UNIQUE_VALUES, highOccuranceCounter);
	#endif
}

/* Prints out the data and resets parameters for a new receiving */
uint8_t receive_complete(void) {
	if(highCount >= NUMBER_OF_PULSES) highCount = 0;
	if(lowCount >= NUMBER_OF_PULSES) lowCount = 0;
	if (startFlag && (TIFR1 & (1<<TOV1))){ //Check that timer has overflowed (thus no more signal) and we have started receiving
		startFlag = 0;
		//TCCR0B = 0; //Kill the timer
		//pulseNumber = 0;
		lowCount = highCount = 0;
		
		cli();
		/* Swap modes into transmission */
		//PORTB ^= (1<<PINB1);
		
		/*if(!RETRIEVE_BIT(state, MODE)) enable_int0();
		else disable_int0();*/
		//disable_int0();
		
		#ifdef DEBUG_PRINT
		usart_transmit('L');
		usart_transmit('\n');
		usart_transmit('\r');
		//usart_transmit(' ');
		uint16_t countaaa = 0;
		for (int i = 0; i < NUMBER_OF_PULSES; i++) {
			uint32_t powerOfTen = 10000;
			for (int j = 4; j >= 0; j--) {
				usart_transmit((unsigned char) 48 + ((lowTime[i] / powerOfTen) % 10)); 
				powerOfTen /= 10;
			}
			usart_transmit('-');
			countaaa++;
			if (!(countaaa % 26)) {
				usart_transmit('\n');
				usart_transmit('\r');
			}
		}
		usart_transmit('\n');
		usart_transmit('\r');
		usart_transmit('\n');
		
		usart_transmit('H');
		usart_transmit('\n');
		usart_transmit('\r');
		countaaa = 0;
		for (int i = 0; i < NUMBER_OF_PULSES; i++) {
			uint32_t powerOfTen = 10000;
			for (int j = 4; j >= 0; j--) {
				usart_transmit((unsigned char) 48 + ((highTime[i] / powerOfTen) % 10));
				powerOfTen /= 10;
			}
			usart_transmit('-');
			countaaa++;
			if (!(countaaa % 26)) {
				usart_transmit('\n');
				usart_transmit('\r');
			}
		}
		usart_transmit('\n');
		usart_transmit('\r');
		usart_transmit('\n');
		#endif

////////////////////////////////////////////////////////////////////////////////////////////////		
		for (uint8_t c = 0; c < NUMBER_OF_PULSES; c++) {
			if (highTime[c] > HUFFMAN_THRESHOLD) highTime[c] = HUFFMAN_THRESHOLD;
			if (lowTime[c] > HUFFMAN_THRESHOLD) lowTime[c] = HUFFMAN_THRESHOLD;
		}
/////////////////////////////////////////////////////////////////////////////////////////////
		
		array_sorter(); //This is to get the unique time values and frequencies
		
		#ifdef DEBUG_PRINT
		//usart_transmit('X'); next_line(); print_uint16_t(rx_or_tx); line_break();
		#endif
		//SET_BIT(state, MODE); //Go to TX mode
		//CLEAR_BIT(PORTB, PINB1); CLEAR_BIT(PORTB, PINB2);
		
		sei();
		return 1;
	}
	return 0;
}

/* Send the currently saved data out */
void send_data(void) {
	cli();
	uint16_t temp;
	for (int i = 0; i < NUMBER_OF_PULSES; i++) {
		temp = highTime[i];
		TCNT1 = 0; //Clear timer 1
		while (temp > TCNT1); //Wait for the alloted period
		
		temp = lowTime[i];
		TCNT1 = 0; //Clear timer 1
		start_timer2();
		while (temp > TCNT1); //Wait for the alloted period
		stop_timer2();
	}
	sei();
}

/* Initialises the hardware for the device */
void init_hardware(void) {
	//DDRB = (1<<1);
	//PORTB = 0;
	PRR = (1<<PRADC) | (1<<PRTWI); //We don't use ADC or TWI, so turn them off
	ACSR = (1<<ACD); //Disable analogue comparator, as we don't use it
	
	init_timer1(); //Initialise and start the timer
	init_timer2();
	init_timer0();
	init_serial();
	init_external_interrupt();
	disable_int0();
	init_watchdog_timer();
	
	DDRB |= (1<<PINB2) | (1<<PINB1); //For the status LEDs
	
	
	sei(); //Enable global interrupts (timer needs this)
}

/* Enable int0 to allow data to be received */
void enable_int0(void) {
	(SET_BIT(EIFR, INTF0)); //Clear interrupt flag
	EIMSK |= (1<<INT0);
}

/* Disable int0 to prevent data erasing */
void disable_int0(void) {
	EIMSK &= ~(1<<INT0);
}

/* Disable the signal buttons (PCINT) */
void disable_pcint1(void) {
	PCICR = 0;
}

/* Enable the signal buttons (PCINT) */
void enable_pcint1(void) {
	//PCICR = (1<<PCIE1);
}

/* Initialises the external interrupt on INT0 */
void init_external_interrupt(void) {
	EICRA = (1<<ISC11) | (1<<ISC10) | (1 << ISC00); //Any edge triggers for INT0, only rising for INT1
	EIMSK = (1<<INT1) | (1 << INT0); //Use INT0
	
	//PCINT11
	/*PCICR = (1<<PCIE1) | (1<<PCIE0); //Enable PCINT from 8 to 14 and PCINT0 for the mode switch
	PCMSK0 = (1<<PCINT0);
	PCMSK1 = (1<<PCINT13) | (1<<PCINT12) | (1<<PCINT11) | (1<<PCINT10) | (1<<PCINT9);
	*/
}

/* Transmits a byte via serial */
void usart_transmit(unsigned char byte){
	/* Wait for empty transfer buffer */
	while (!(UCSR0A & (1<<UDRE0)));
	
	/* Put the byte into the buffer and send it out */
	UDR0 = byte;
}

/* Initialises UART0 for serial transfer */
void init_serial (void){
	UBRR0 = 25; //Formula for asynchronous normal mode on page 173 
	UCSR0B = (1<<TXEN0); //Enable transfer of data
}

/* This timer does not get started in its initialisation */
void init_timer2(void){
	DDRB |= (1<<PINB3); //This is for OC2A
	TCNT2 = 0; //Clear counter
	OCR2A = 108; //Approx 37-38kHz
	TCCR2A = (1<<COM2A0) | (1<<WGM21); //Toggle OC2A on compare match and choose CTC mode
}

/* Stops the waveform generation timer */
void stop_timer2(void){
	/* I believe that CTC mode makes OC2A override any PORTB meddlings */
	TCCR2B = 0; //Stop the timer
	if(PINB & (1<<PINB3)) TCCR2B = (1<<FOC2A); //Force the pin low if it's high
	TCNT2 = 0; //Clear the timer
}

/* Starts the waveform generation timer */
void start_timer2(void){
	TCCR2B = (1<<CS20); //Unity prescale. This starts the timer
}

/* Initialises timer 0 to run at 125kHz */
void init_timer1(void) {	
	/* Clear the timer */
	TCNT1 = 0;
	TCCR1B = (1<<CS11) | (1<<CS10); //Set to normal mode (top = 0xFFFF) and prescale by 64 (clocks every 8 microseconds)
}

/* Put the microcontroller to sleep to conserve power */
void sleep(void) {
	cli();
	uint8_t savedDDRB, savedDDRC, savedDDRD;
	uint8_t savedPORTB, savedPORTC, savedPORTD;
	
	savedDDRB = DDRB;	savedDDRC = DDRC;	savedDDRD = DDRD;
	savedPORTB = PORTB; savedPORTC = PORTC; savedPORTD = PORTD;
	
	PORTB = PORTC = PORTD = 0; //Set all outputs to zero
	DDRB = DDRC = DDRD = 0; //Set all pins to inputs
	
	PRR |= (1<<PRUSART0) | (1<<PRTIM2) | (1<<PRTIM1) | (1<<PRTIM0); //Turn off USART0, TIM2, TIM1 and TIM0 (everything should be off now)
	
	SMCR = (1<<SM1) | (1<<SE); //Power down mode and enable
	disable_int0();
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	//sleep_bod_disable(); //This isn't available on the 88A
	sei();
	
	sleep_cpu(); //This is where we actually sleep
	//sleep_mode();
	
	sleep_disable();
	// Restore turned off modules 
	PRR &= ~(1<<PRUSART0) & ~(1<<PRTIM2) & ~(1<<PRTIM1) & ~(1<<PRTIM0);
	
	// Restore DDRx and PORTx 
	PORTB = savedPORTB;
	PORTC = savedPORTC;
	PORTD = savedPORTD;
	DDRB = savedDDRB;
	DDRC = savedDDRC;
	DDRD = savedDDRD;
	
	
	// Restore USART0 
	init_serial();
	init_timer1();
	init_timer2();
	init_external_interrupt();
	//enable_int0();
	
}

/* This interrupt is called by the press of any of the buttons except for the tx/rx toggle */
ISR(INT1_vect) {
	if (CHECK_TOGGLE()) {
		TOGGLE_MODE();
		CLEAR_BIT(PORTB, PINB1);
		CLEAR_BIT(PORTB, PINB2);
		DDRB &= ~((1 << PINB2) | (1 << PINB1));
		disable_int0(); //I believe this is appropriate here to ensure tx isn't interrupted by an incident IR signal
	} else {
		if (RETRIEVE_BIT(state, MODE) == TX_MODE) { //We're in tx mode
			//txFlag = 1;
			SET_BIT(state, TX_FLAG); //We want to transmit!
			//txButton = INVALID_BUTTON;
			//if (!txPressed) {
			//print_string("first time\r\n");
			//We need each button to have a unique number, so we will need to add an offset.
			//This will be called variable_shift
			uint8_t variableShift = 0;
			
			//Changed the bounds here to add more buttons
			//Check PINC (1-5)
			for (uint8_t z = 1; z < 6; z++) {
				if (PINC & (1 << z)) txButton = variableShift;
				variableShift++;
			}
			
			//Check PIND (5-7)
			for (uint8_t z = 5; z <= 7; z++) {
				if (PIND & (1 << z)) txButton = variableShift;
				variableShift++;
			}
			
			//Check PINB (6-7)
			for (uint8_t z = 6; z <= 7; z++) {
				if (PINB & (1 << z)) txButton = variableShift;
				variableShift++;
			}
		} 
	}
}

/* Interrupt vector for toggling between learning and transmitting mode */
/*ISR(PCINT0_vect) {
	modeCounter++;
	if (modeCounter < 2) {
		PORTB ^= (1<<PINB1);
		PORTB &= ~(1<<PINB2);
		rxButton = INVALID_BUTTON;
		rx_or_tx ^= 1;
		if(!rx_or_tx) enable_int0();
		else disable_int0();
	} else {
		modeCounter = 0;
	}
}*/

/* Interrupt vector for telling the microcontroller to send a signal out */
/*ISR(PCINT1_vect) {
	//I'm tentatively leaving this empty and transferring all PCINT control to INT1 as a test
	//Delete this and clean up PCINT control if the test works
	
}*/

ISR(TIMER0_COMPA_vect) {
	rxTicks++;
	
	if (rxTicks > RX_TIMEOUT) {
		breakout++;
		stop_timer0();
	}
}

/* Interrupt vector for capturing the given remote's waveform */
ISR(INT0_vect) {
	//print_string("I'm in INT0\r\n");
	if (!highCount) {
		TIFR1 = (1<<TOV1); //Clear overflow flag
		startFlag = 1;
	}
	
	/* Check INT0 state */
	if (PIND & (1<<PIND2)) { //Rising edge (so we've gone low -> high), so a low period is done. 
		lowTime[lowCount++] = TCNT1; //Save previous low period
		//Maybe I should use one array in case I'm incrementing index weirdly
		
		/* Falling edge (we ////should//// (verify) hit this first due to the sensor being default high).
		 * We've gone high -> low, so save the just completed high period (except if it's the first one)
		 */
	} else { 
		//if (lowCount) { //Ignore the very first time this happens, as it will be from the sensor going low
		highTime[highCount++] = TCNT1; //Save previous high period
		//} 
	}
	
	//PCIFR = (1<<PCIF1);
	TCNT1 = 0;
	//lastInterrupt = ticks; //Save the number of ticks at which this occurred
}

