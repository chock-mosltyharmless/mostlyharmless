#pragma once

#define ED_MAX_LINE_LENGTH 70
#define ED_MAX_NUM_LINES 5000
#define ED_MAX_FILESIZE (ED_MAX_LINE_LENGTH*(ED_MAX_NUM_LINES+1))
#define ED_MAX_FILENAME_LENGTH 1024

// Font size stuff
#define ED_CHAR_TEX_WIDTH 0.0688100961538462f
#define ED_CHAR_TEX_HEIGHT 0.0732421875f
#define ED_CHAR_TEXX_OFFSET 0.017578125f
#define ED_CHAR_TEXY_OFFSET 0.0078125f
#define ED_CHAR_TEXX_BORDER 0.002f
#define ED_CHAR_TEXY_BORDER 0.002f
#define ED_CHAR_WIDTH 0.025f
#define ED_CHAR_HEIGHT 0.07f
#define ED_CHAR_X_BORDER -0.008f
#define ED_CHAR_Y_BORDER 0.003f
#define ED_CHAR_X_OFFSET (-0.99f + (5.0f * ED_CHAR_WIDTH))
#define ED_CHAR_Y_OFFSET -0.925f

// Number of lines that are displayed
#define ED_DISPLAY_LINES 20

// Cursor and text stuff
#define ED_CURSOR_BLINK_SPEED 0.015f
#define ED_TEXT_RED 0.7f
#define ED_TEXT_GREEN 0.9f
#define ED_TEXT_BLUE 1.0f
#define ED_LINENUM_RED 1.0f
#define ED_LINENUM_GREEN 0.9f
#define ED_LINENUM_BLUE 0.7f
#define ED_LINENUM_ALPHA 0.3f
#define ED_CURSOR_X_ADJUST -0.32f

// Number of lines in one page
#define ED_PAGE_LENGTH 10

// Scrolling constants
#define ED_SCROLL_MARGIN 5
#define ED_SCROLL_SPEED 0.003f

#include "ShaderManager.h"
#include "TextureManager.h"

/**
 * The Editor is the class that is used for text editing.
 * Because I am a lazy bastard, I combine backend and frontend(s)...
 * The underlying structure is a 2D array, that is completely filled with blanks
 * before the newlines. That way I can just pass it as a normal text... :-)
 * I have a maximum Line length...
 * The end of the buffer is also 0-terminated (additional to a number of lines counter)
 * Indentation is automatic (I will just cound '{' and '}' and recognize ';')
 * I will go *nix on the line endings and just do \n....
 */

class Editor
{
public:
	Editor(void);
	~Editor(void);

public: // functions
	// Returns 0 on success.
	int init(ShaderManager *shaderMgr, TextureManager *textureMgr, char *errorText);

	// Returns 0 on success.
	int loadText(const char *filename, char *errorText);

	// Draw the text. There is no room for error here.
	void render(long iTime);

	// Cursor movement using keys
	void moveCursor(WPARAM vKey);

	// Special characters (return, del, ...)
	void controlCharacter(WPARAM vKey);

	// A key was pressed, it can be put in...
	void putCharacter(WPARAM key);

private: // functions
	void clear(void);
	void createFontTable(void);
	// Uses scrolling position, assumes glBegin(GL_QUADS), correct program/texture.
	// It does not change color.
	void drawChar(float xPos, float yPos, unsigned char textBit);
	int getLineLength(int line);

private: // data
	unsigned char text[ED_MAX_NUM_LINES+1][ED_MAX_LINE_LENGTH+1];
	char filename[ED_MAX_FILENAME_LENGTH+1];
	unsigned char textBuffer[ED_MAX_FILESIZE+1];
	int numLines;
	ShaderManager *shaderManager;
	TextureManager *textureManager;

	// For the timing of scrolling and stuff
	long lastRenderTime;

	// Location of the cursor (for editing)
	int cursorPos[2];
	int cursorWantX; // The cursor 'wants to be here'

	// Font stuff
	int charIPos[256][2];
	float scrollPos;
	float scrollDestination;
};