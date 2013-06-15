#pragma once

#define ED_MAX_LINE_LENGTH 80
#define ED_MAX_NUM_LINES 5000
#define ED_MAX_FILESIZE (ED_MAX_LINE_LENGTH*(ED_MAX_NUM_LINES+1))
#define ED_MAX_FILENAME_LENGTH 1024

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

private: // functions
	void clear(void);

private: // data
	char text[ED_MAX_NUM_LINES+1][ED_MAX_LINE_LENGTH+1];
	char filename[ED_MAX_FILENAME_LENGTH+1];
	char textBuffer[ED_MAX_FILESIZE+1];
	int numLines;
	ShaderManager *shaderManager;
	TextureManager *textureManager;
};

