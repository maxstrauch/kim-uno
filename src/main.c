/**
	KIM Uno
	(c) 2018 - 2019. Maximilian Strauch.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "fibonacci.h"

// Uncomment the following line if you want a clean memory
#define WITH_FIBONACCI 1

#define BUSY_WAIT(cond) while (cond) { _delay_ms(10); }

#define MODE_NO_CHANGE -1
#define MODE_MAINMENU 0
#define MODE_DATAINPUT 1
#define MODE_ADDRINPUT 2
#define MODE_EMULATOR 3

void setMode(int newMode);
void scan_keyboard();
int getkey();


////////////////////////////////////////////////////////////////////////////////
// Constants & Variables
////////////////////////////////////////////////////////////////////////////////


// Contains the data to draw numbers onto the seven segment displays
unsigned const PROGMEM char segmentTable[20] = {
	// Regular symbols: 0...9 A...F
	0x02, 0x9e, 0x24, 0x0c, 0x98, 0x48, 0x40, 0x1e,
	0x00, 0x08, 0x10, 0xc0, 0x62, 0x84, 0x60, 0x70,
	
	// Special symbols
	/* 0x10:*/ 0x62, // [
	/* 0x11:*/ 0x0e, // ]
	/* 0x12:*/ 0x6e, // =
	/* 0x13:*/ 0xfc, // -
};

// Contains the data to display on the six seven segment displays
// The value can be greater than 0xf to show one of the four symbols from
// segmentTable (until 0x13). A value greater than that or 0xff turns off the
// segment
unsigned char displayData[6] = {
	0x0, // most right one
	0x0,
	0x0,
	0x0,
	0x0,
	0x0  // most left one
};

// "Normal keyboard values"; if 0x4f is pressed (2nd), the numeric value of
// the next pressed key is used as an index for keyboard2nd and the value at
// this position is returned
int keyboardMatrix[4][5] = {
	{ 0xc, 0xd, 0xe, 0xf, 0x4f },
	{ 0x8, 0x9, 0xa, 0xb, 0x3f },
	{ 0x4, 0x5, 0x6, 0x7, 0x2f },
	{ 0x0, 0x1, 0x2, 0x3, 0x1f }
};

// Secondary keyboard matrix
int keyboard2nd[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x8f, 0x7f, 0x6f, 0x5f	
};

// Holds the current state of the keyboard
struct kbrdStruct {
	int currentCol;		// Current col which is scanned
	int hadKeyPress;	// 1 if there had been a keypress (debouncing)
	int isKeyPressed;	// 1 if key is pressed, otherwise 0
	int row;			// Currently pressed row
	int currentKeyCode; // Currently pressed key
};

struct kbrdStruct kbrd;


////////////////////////////////////////////////////////////////////////////////
// Interrupt handler
////////////////////////////////////////////////////////////////////////////////


// Holds the index of the digit (0 - 6) which is currently going to be
// displayed "painted"
int currentPaintedDigit = 0;

// Interrupt service routine, responsible for "painting" the current 
// digit indicated by currentPaintedDigit
ISR(TIMER1_COMPA_vect){
	PORTC = 0x00; // Turn of all digits

	int toDisplay = displayData[currentPaintedDigit];

	if (toDisplay > 20) {
		// Turn off the segment
		PORTB = 0xfe | (PORTB & 0x1);
	} else {
		// Get the byte containing the settings to show the
		// value toDisplay on the seven segment display
		int data = pgm_read_byte(&segmentTable[toDisplay]);
		PORTB = data | (PORTB & 0x1);
	}

	// Turn on the current digit and advance it for the next time
	PORTC = (1 << currentPaintedDigit);
	currentPaintedDigit = (currentPaintedDigit + 1) % 6;

	// Perform a quick keyboard scan
	scan_keyboard();
}

// Interrupt routine to scan the keyboard and detect keypresses
void scan_keyboard() {
	int hasKeyPress = -1;
	
	// Check if any keyboard row is low = a key has been pressed
	if ((PIND & 0xf) == 0xe) {
		hasKeyPress = 1;
		kbrd.row = 0;
	} else if ((PIND & 0xf) == 0xd) {
		hasKeyPress = 1;
		kbrd.row = 1;
	} else if ((PIND & 0xf) == 0xb) {
		hasKeyPress = 1;
		kbrd.row = 2;
	} else if ((PIND & 0xf) == 0x7) {
		hasKeyPress = 1;
		kbrd.row = 3;
	} else {
		hasKeyPress = 0;
		kbrd.row = -1;
	}
	
	// Perform a debouncing by setting state variables and checking
	// them the next call of this function when some time has passed
	if (hasKeyPress && !kbrd.hadKeyPress) {
		kbrd.hadKeyPress = 1;
		return;
	} else if (hasKeyPress && kbrd.hadKeyPress) {
		if (!kbrd.isKeyPressed) {
			kbrd.currentKeyCode = keyboardMatrix[kbrd.row][kbrd.currentCol];
			kbrd.isKeyPressed = 1;
		}
		return;
	} else {
		kbrd.hadKeyPress = 0;
		kbrd.currentKeyCode = 0xff;
		kbrd.isKeyPressed = 0;
	}
	
	// Activate the current column
	kbrd.currentCol = (kbrd.currentCol + 1) % 5;
	switch (kbrd.currentCol) {
		default:
		case 0:
			PORTD = 0xe0 | (PORTD & 0x0f);
			PORTB |= (1 << PB0);
			break;
		case 1:
			PORTD = 0xd0 | (PORTD & 0x0f);
			PORTB |= (1 << PB0);
			break;
		case 2:
			PORTD = 0xb0 | (PORTD & 0x0f);
			PORTB |= (1 << PB0);
			break;
		case 3:
			PORTD = 0x70 | (PORTD & 0x0f);
			PORTB |= (1 << PB0);
			break;
		case 4:
			PORTD = 0xf0 | (PORTD & 0x0f);
			PORTB &= ~(1 << PB0);
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard interfacing functions
////////////////////////////////////////////////////////////////////////////////


// Returns the currently pressed keycode and blocks until a key has been 
// released
int getkey() {
	int code;
	BUSY_WAIT((code = kbrd.currentKeyCode) == 0xff);
	
	if (code == 0x4f) { // 2nd
		unsigned char tmp[6] = {};
		
		memcpy(tmp, displayData, 6);
		memset(displayData, 0x13, 6);

		BUSY_WAIT(kbrd.isKeyPressed);
		BUSY_WAIT((code = kbrd.currentKeyCode) == 0xff);
		code = keyboard2nd[code & 0xf];
		
		BUSY_WAIT(kbrd.isKeyPressed);
		memcpy(displayData, tmp, 6);
	} else {
		BUSY_WAIT(kbrd.isKeyPressed);
	}

	return code;
}

// Returns the currently pressed key code or 0xff if no key is pressed
// currently (non-blocking)
int getkey_simple() {
	return kbrd.currentKeyCode;
}


////////////////////////////////////////////////////////////////////////////////
// Memory interfacing functions
////////////////////////////////////////////////////////////////////////////////


// The simulated main memory of the CPU
unsigned char mem[1024];

// Reads a byte at memory location addr
unsigned char memory_read(unsigned char addr) {
	switch (addr) {
		case 1:
			return (displayData[0]) & 0xf;
		case 2:
			return (displayData[1]) & 0xf;
		case 3:
			return (displayData[2]) & 0xf;
		case 4:
			return (displayData[3]) & 0xf;
		case 5:
			return (displayData[4]) & 0xf;
		case 6:
			return (displayData[5]) & 0xf;
			
		case 7:
			return ((displayData[1] << 4) & 0xf) | (displayData[0]) & 0xf;
		case 8:
			return ((displayData[3] << 4) & 0xf) | (displayData[2]) & 0xf;
		case 9:
			return ((displayData[5] << 4) & 0xf) | (displayData[4]) & 0xf;
			
		default:
			return mem[addr];
	}
}

// Writes the byte data at memory location addr
void memory_write(unsigned char addr, unsigned char data) {
	switch (addr) {
		case 1:
			displayData[0] = data;
			return;
		case 2:
			displayData[1] = data;
			return;
		case 3:
			displayData[2] = data;
			return;
		case 4:
			displayData[3] = data;
			return;
		case 5:
			displayData[4] = data;
			return;
		case 6:
			displayData[5] = data;
			return;
			
		case 7:
			displayData[0] = data & 0xf;
			displayData[1] = (data >> 4) & 0xf;
			return;
		case 8:
			displayData[2] = data & 0xf;
			displayData[3] = (data >> 4) & 0xf;
			return;
		case 9:
			displayData[4] = data & 0xf;
			displayData[5] = (data >> 4) & 0xf;
			return;	
				
		default:
			mem[addr] = data;
			return;
	}
}

////////////////////////////////////////////////////////////////////////////////
// "Operating System"
////////////////////////////////////////////////////////////////////////////////

// Address pointer, used by the different menu functions (following)
unsigned long addrPtr = 0;

// Function to draw & handle the main menu
int main_menu() {
    int key = 0xff;
    int memVal;
	
	displayData[0] = 0x11;
	displayData[1] = 0x12;
	displayData[2] = 0x12;
	displayData[3] = 0x12;
	displayData[4] = 0x12;
	displayData[5] = 0x10;

	key = getkey();

	if (key == 0x8f) {
		return MODE_DATAINPUT;
	} else if (key == 0x7f) {
		return MODE_ADDRINPUT;
	} else if (key == 0x1f) {
		return MODE_EMULATOR;
	}

	return MODE_NO_CHANGE;
}

// Data input function to enter data into the main memory (press 2nd + C)
int data_input() {
	int shiftPos = 0;
	int key;
	int memVal;

	while (1) {
		int key = 0xff;
		int memVal;
		
		memVal = memory_read(addrPtr);
		
		displayData[0] = memVal & 0xf;
		displayData[1] = (memVal >> 4) & 0xf;
		displayData[2] = addrPtr & 0xf;
		displayData[3] = (addrPtr >> 4) & 0xf;
		displayData[4] = (addrPtr >> 8) & 0xf;
		displayData[5] = (addrPtr >> 12) & 0xf;
		
		key = getkey();
		
		if (key == 0x3f) {
			addrPtr = (addrPtr + 1) & 0xffff;
			shiftPos = 0;
		} else if (key == 0x2f) {
			addrPtr = (addrPtr - 1) & 0xffff;
			shiftPos = 0;
		} else if (key <= 0xf) {
			if (!shiftPos) {
				memory_write(addrPtr, key);
				shiftPos = 1;
			} else {
				memVal = memory_read(addrPtr);
				memory_write(addrPtr, (memVal << 4 | key) & 0xff);
				shiftPos = 0;
			}
		} else if (key >= 0x5f && key != 0x8f) {
			return MODE_MAINMENU;
		} else if (key == 0x1f) {
			return MODE_EMULATOR;
		}
	}
}

// Address input function to enter an address (press 2nd + D)
int addr_input() {
	int key = 0xff;
	int shiftPos = 0;
	
	while (1) {
		displayData[0] = 0xff;
		displayData[1] = 0xff;
		displayData[2] = addrPtr & 0xf;
		displayData[3] = (addrPtr >> 4) & 0xf;
		displayData[4] = (addrPtr >> 8) & 0xf;
		displayData[5] = (addrPtr >> 12) & 0xf;
		
		key = getkey();
		
		if (key == 0x3f) {
			addrPtr = (addrPtr + 1) & 0xffff;
		} else if (key == 0x2f) {
			addrPtr = (addrPtr - 1) & 0xffff;
		} else if (key <= 0xf) {
			switch (shiftPos) {
				case 0:
					shiftPos = 1;
					addrPtr = (addrPtr & 0x0fff) | ((key << 12) & 0xf000);
					break;
				case 1:
					shiftPos = 2;
					addrPtr = (addrPtr & 0xf0ff) | ((key << 8) & 0x0f00);
					break;
				case 2:
					shiftPos = 3;
					addrPtr = (addrPtr & 0xff0f) | ((key << 4) & 0x00f0);
					break;
				case 3:
					shiftPos = 0;
					addrPtr = (addrPtr & 0xfff0) | (key & 0x000f);
					break;
			}
		} else if (key >= 0x5f && key != 0x7f) {
			return MODE_MAINMENU;
		} else if (key == 0x1f) {
			return MODE_EMULATOR;
		}
	}
}

// SUBLEQ single instruction CPU emulation function
void emulator() {
	displayData[0] = 0xff;
	displayData[1] = 0xff;
	displayData[2] = 0xff;
	displayData[3] = 0xff;
	displayData[4] = 0xff;
	displayData[5] = 0xff;
	
	unsigned char PC = 10; /* Program Counter */
	unsigned char a, b, c, ma, mb, ret;
	
	while(PC > 0) {
		a = memory_read(PC++);
		b = memory_read(PC++);
		c = memory_read(PC++);
		
		ma = memory_read(a);
		mb = memory_read(b);

		ret = (mb - ma) & 0xff;
		
		memory_write(b, ret);
		
		if (getkey_simple() == 0x1f) break;
		
		if (ret <= 0) {
			PC = c;
		}
		
		if (getkey_simple() == 0x1f) break;
	}
	
	// End of program
	
	BUSY_WAIT(kbrd.isKeyPressed);
	getkey();
}


////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////


int main() {

	#ifdef WITH_FIBONACCI
		// Load the demo program
		for (int i = 0; i < 0xff; i++) {
			mem[i] = pgm_read_byte(&prg[i]);
		}
	#endif
	
	// Set keyboard struct initial values
	kbrd.currentKeyCode = 0xff;
	kbrd.currentCol = 0;
	
	// Configure inputs & outputs (0 = INPUT, 1 = OUTPUT)
	// for keyboard and seven segment display
	DDRC = 0x3f;
	DDRB = 0xff;
	DDRD = 0xf0;
	
	PORTC |= (1 << PC0); 
	PORTB = 0xff;
	PORTD = 0xff; // Set all cols low _ enable pull-up for rows
    
    // Enable the timer to interrupt to readraw the seven segments
	TCCR1B = (1 << WGM12) | (1 << CS01) | (1 << CS00);
	TCNT1  = 0;
	OCR1A = 249;
	TIMSK1 |= (1 << OCIE1A);

	sei();
  
  	// Start the main menu loop
	int mode = MODE_MAINMENU;
	int retval = -1;

	while(1) {
		switch (mode) {
			default:
			case MODE_MAINMENU:
				retval = main_menu();
				break;
			
			case MODE_DATAINPUT:
				retval = data_input();
				break;
				
			case MODE_ADDRINPUT:
				retval = addr_input();
				break;
				
			case MODE_EMULATOR:
				emulator();
				retval = MODE_MAINMENU;
				break;
		}

		if (retval > -1) {
			mode = retval;
		}
	}

	return 0;
}
