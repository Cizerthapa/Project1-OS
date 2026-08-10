#include "project1.h"
#include "project2.h"

// Define our constants that will be widely used
#define TEXT_COLOR 0x07
#define VIDEO_MEM 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// Track the current cursor's row and column
static int cursorCol = 0;
static int cursorRow = 0;

#if PROJECT == 1
// This function runs when compiled with the #define PROJECT 1 flag
int kernel()
{
	// Print hello world to the display
	return 0;
}
#endif

// Setting the cursor does not display anything visually
// Setting the cursor is simply used by putchar() to find where to print next
// This can also be set independently of putchar() to print at any x, y coordinate on the screen
int setcursor(int x, int y)
{
	(void)x; // Remove this when implementing your function
	(void)y; // Remove this when implementing your function
	(void)cursorCol; // Remove this when implementing your function
	(void)cursorRow; // Remove this when implementing your function
	return 0;
}

// Using a pointer to video memory we can put characters to the display
// Every two addresses contain a character and a color
char putchar(char character)
{
	return character;
}

// Print the character array (string) using putchar()
// Print until we find a NULL terminator (0)
int print(char string[]) 
{
	(void)string; // Remove this when implementing your function
	// Use character count to track how many characters we print
	int characterCount = 0;
	return characterCount;
}

// Clear the screen by placing a ' ' character in every character location
void clearscreen()
{
	return;
}