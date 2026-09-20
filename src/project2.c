#include "project2.h"

// Define a keymap to convert keyboard scancodes to ASCII
static char keymap[128] = {
	[0x1E] = 'a',
	[0x30] = 'b',
	[0x2E] = 'c',
	[0x20] = 'd',
	[0x12] = 'e',
	[0x21] = 'f',
	[0x22] = 'g',
	[0x23] = 'h',
	[0x17] = 'i',
	[0x24] = 'j',
	[0x25] = 'k',
	[0x26] = 'l',
	[0x32] = 'm',
	[0x31] = 'n',
	[0x18] = 'o',
	[0x19] = 'p',
	[0x10] = 'q',
	[0x13] = 'r',
	[0x1F] = 's',
	[0x14] = 't',
	[0x16] = 'u',
	[0x2F] = 'v',
	[0x2C] = 'w',
	[0x2B] = 'x',
	[0x15] = 'y',
	[0x2D] = 'z',
	[0x02] = '1',
	[0x03] = '2',
	[0x04] = '3',
	[0x05] = '4',
	[0x06] = '5',
	[0x07] = '6',
	[0x08] = '7',
	[0x09] = '8',
	[0x0A] = '9',
	[0x0B] = '0',
	[0x1C] = '\n',
	[0x39] = ' ',
	[0x0E] = '\t',
};

#if PROJECT == 2
// This function runs when compiled with the #define PROJECT 2 flag for make
int kernel()
{
	// Ask the user to type in stuff forever
	char buffer[100];
	while (1) {
		print("Please type something: ");
		scan(buffer);
		print("You typed: ");
		print(buffer);
		print("\n");
	}
	return 0;
}
#endif

// Gets the character from the keyboard
// This is a blocking function that does not use interrupts
// Only I/O ports and polling are used
char getchar()
{
	uint8 scancode;

	// 0x64 = keyboard STATUS port
	// read to check if the keyboard has sent us data
	// Bit 0 = 1 means "data is ready to be read"
	while ((inb(0x64) & 1) == 0);

	// 0x60 = keyboard DATA port
	// keyboard ready, read scancode from here
	scancode = inb(0x60);

	// If bit 7 = 1, it key release - ignore it and try again
	if (scancode & 0x80)
		return getchar();

	// Returning ASCII character
	return keymap[scancode];
}

// Read characters from the keyboard until the user hits the enter key
// Accepts a character array to fill with characters
// Terminates string with NULL terminator when done
void scan(char string[])
{
	int i = 0;
	char c;

	// Keep reading characters until Enter is pressed
	c = getchar();
	while (c != '\n') {
		putchar(c);    // Shows the character on screen
		string[i] = c; // Store it in the string
		i++;
		c = getchar();
	}

	// End the string with a null terminator
	string[i] = '\0';
}

void scroll(int rows)
{
	
}