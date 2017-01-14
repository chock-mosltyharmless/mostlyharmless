#include "StdAfx.h"
#include "Editor.h"
#include "Configuration.h"
#include "GLNames.h"
#include "glext.h"

Editor::Editor(void)
{
	lastRenderTime = 0;
	// Undo
	nextFileHistory = 0; // position where to next store a file history
	lastFileHistory = -1; // Where to go when you press CTRL-Z (set to -1 if no more undo possible)
	firstFileHistory = -1; // Do no go before this position
	clear();
}

Editor::~Editor(void)
{
}

void Editor::clear(void)
{
	numLines = 1; // So that I can do anything
	for (int k = 0; k < ED_MAX_NUM_LINES; k++)
	{
		text[k][0] = 0;
	}
	// One empty line
	for (int k = 0; k < ED_MAX_LINE_LENGTH; k++)
	{
		text[0][k] = ' ';
	}
	text[0][ED_MAX_LINE_LENGTH] = '\n';
	cursorPos[0] = 0;
	cursorPos[1] = 0;
	cursorWantX = 0;
	scrollPos = 0.0f;
	scrollDestination = 0.0f;
	updateIndentation();

	// error display
	isErrorFading = true;
	displayedError[0] = 0;
	errorDisplayAlpha = 0.0f;
	isTextFading = true;
	textDisplayAlpha = 0.0f;
}

int Editor::init(ShaderManager *shaderMgr, TextureManager *textureMgr, char *errorText)
{
	shaderManager = shaderMgr;
	textureManager = textureMgr;
	createFontTable();
	return 0;
}

void Editor::render(long iTime)
{
	// The scrolling stuff comes first:
	int iScrollPos = (int)scrollPos;
	float dScrollPos = scrollPos - iScrollPos;
	float scrollDifference = 0.0f;
	if (cursorPos[1] < iScrollPos + ED_SCROLL_MARGIN)
	{
		scrollDifference = cursorPos[1] - (scrollPos + ED_SCROLL_MARGIN);
	}
	if (cursorPos[1] > iScrollPos + ED_DISPLAY_LINES - ED_SCROLL_MARGIN)
	{
		scrollDifference = cursorPos[1] - (scrollPos + ED_DISPLAY_LINES - ED_SCROLL_MARGIN);
	}
	// Scroll to next location if applicable
	if (scrollDifference == 0.0f)
	{
		if (dScrollPos < 0.5f)
		{
			scrollDifference = -dScrollPos;
		}
		else
		{
			scrollDifference = 1.0f - dScrollPos;
		}
	}
	float scrollSpeed = ED_SCROLL_SPEED * scrollDifference;
	float scrollAdd = scrollSpeed * (iTime - lastRenderTime);
	if (fabsf(scrollAdd) > fabsf(scrollDifference)) scrollAdd = scrollDifference;
	scrollPos += scrollAdd;
	if (scrollPos < 0) scrollPos = 0;

	// update alpha
	if (isErrorFading)
	{
		errorDisplayAlpha *= (float)exp(-(float)(iTime - lastRenderTime) * ED_FADE_SPEED);
		errorDisplayAlpha = errorDisplayAlpha > 1.0f ? 1.0f : errorDisplayAlpha;
		errorDisplayAlpha = errorDisplayAlpha < 0.0f ? 0.0f : errorDisplayAlpha;
	}
	if (isTextFading)
	{
		textDisplayAlpha *= (float)exp(-(float)(iTime - lastRenderTime) * ED_FADE_SPEED);
		textDisplayAlpha = textDisplayAlpha > 1.0f ? 1.0f : textDisplayAlpha;
		textDisplayAlpha = textDisplayAlpha < 0.0f ? 0.0f : textDisplayAlpha;
	}


	// So far I just render from the top
	// There is no room for error here!
	GLuint texID;
	GLuint progID;
	char errorString[MAX_ERROR_LENGTH + 1];
	textureManager->getTextureID("font_512.tga", &texID, errorString);
	glBindTexture(GL_TEXTURE_2D, texID);
	shaderManager->getProgramID("SimpleTexture.gprg", &progID, errorString);
	glUseProgram(progID);	

	// For now I am dumb.
	// TODO: I only draw in a sensible range!!!!
	//       And fade out whatever...
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	// Draw text
	for (int y = iScrollPos; y < ED_DISPLAY_LINES + iScrollPos + 1 && y < numLines; y++)
	{
		float yDisplay = y - scrollPos;
		float alpha = textDisplayAlpha;
		if (y == iScrollPos) alpha *= 1.0f - dScrollPos;
		if (y == ED_DISPLAY_LINES + iScrollPos) alpha *= dScrollPos;

		// Draw line number:
		if (cursorPos[1] == y)
		{
			glColor4f(ED_LINENUM_RED, ED_LINENUM_GREEN, ED_LINENUM_BLUE, alpha);
		}
		else
		{
			glColor4f(ED_LINENUM_RED, ED_LINENUM_GREEN, ED_LINENUM_BLUE, ED_LINENUM_ALPHA * alpha);
		}
		if (y+1 > 999) drawChar(-5.0f, yDisplay, (((y+1)/1000) % 10) + '0');
		if (y+1 > 99) drawChar(-4.0f, yDisplay, (((y+1)/100) % 10) + '0');
		if (y+1 > 9) drawChar(-3.0f, yDisplay, (((y+1)/10) % 10) + '0');
		drawChar(-2.0f, yDisplay, (((y+1)/1) % 10) + '0');

		if (isTextFading)
		{
			glColor4f(ED_OK_RED, ED_OK_GREEN, ED_OK_BLUE, alpha);
		}
		else
		{
			glColor4f(ED_TEXT_RED, ED_TEXT_GREEN, ED_TEXT_BLUE, alpha);
		}
		float indent = indentation[y] * ED_INDENTATION_WIDTH / aspectRatio;
		for (int x = 0; x < ED_MAX_LINE_LENGTH; x++)
		{
			if (text[y][x] != ' ')
			{
				drawChar((float)x + indent, yDisplay, text[y][x]);
			}
		}
	}

	// Render the cursor
	float alpha = 0.5f * sinf((float)iTime * ED_CURSOR_BLINK_SPEED) + 0.5f;
	alpha *= textDisplayAlpha;
	glColor4f(ED_TEXT_RED, ED_TEXT_GREEN, ED_TEXT_BLUE, alpha);
	float indent = indentation[cursorPos[1]] * ED_INDENTATION_WIDTH / aspectRatio;
	// '\r' Is the special char that is used for the cursor
	drawChar((float)cursorPos[0] + indent, cursorPos[1] - scrollPos, '\r');

	// render the error text
	int errorTextPos = 0;
	int errorTextX = 0;
	int errorTextY = 0;
	bool isError = false; // Only draw if it really is an error.
	int lineCharPos = 0; // character position inside the line
	while (displayedError[errorTextPos] != 0)
	{
		float yDisplay = (float)errorTextY;
		float alpha = errorDisplayAlpha;

		if (isErrorFading)
		{
			glColor4f(ED_OK_RED, ED_OK_GREEN, ED_OK_BLUE, alpha);
		}
		else
		{
			glColor4f(ED_ERROR_RED, ED_ERROR_GREEN, ED_ERROR_BLUE, alpha);
		}
		for (int x = 0; x < ED_MAX_LINE_LENGTH; x++)
		{
			if (displayedError[errorTextPos] != ' ' &&
				displayedError[errorTextPos] != '\r' &&
				displayedError[errorTextPos] != '\t' &&
				isError)
			{
				drawChar((float)errorTextX, yDisplay + ED_ERROR_Y_OFFSET,
					     displayedError[errorTextPos]);
			}
		}
		if (lineCharPos == 4 && (displayedError[errorTextPos] == 'r' || displayedError[errorTextPos] == 'R'))
		{
			isError = true;
		}

		lineCharPos++;

		if (isError) errorTextX++;
		
		if (displayedError[errorTextPos+1] == '\n')
		{
			errorTextX = 0;
			if (isError) errorTextY++;
			errorTextPos++;
			isError = false; // assume no error.
			lineCharPos = 0;
		}
		if (errorTextX >= ED_MAX_LINE_LENGTH)
		{
			errorTextX = 0;
			errorTextY++;
		}
		errorTextPos++;
	}

	glEnd();

	lastRenderTime = iTime;
}

void Editor::drawChar(float xPos, float yPos, unsigned char textBit)
{
	// Special handling for the cursor character
	float xAdjust = 0.0f;
	if (textBit == '\r')
	{
		xAdjust = ED_CURSOR_X_ADJUST * ED_CHAR_WIDTH / aspectRatio;
		textBit = '|';
	}

	int texX = charIPos[textBit][0];
	int texY = charIPos[textBit][1];

	float texLeft = ED_CHAR_TEXX_OFFSET +
		charIPos[textBit][0] * ED_CHAR_TEX_WIDTH +
		ED_CHAR_TEXX_BORDER;
	float texRight = texLeft + ED_CHAR_TEX_WIDTH - 2.0f * ED_CHAR_TEXX_BORDER;
	float texTop = ED_CHAR_TEXY_OFFSET +
		charIPos[textBit][1] * ED_CHAR_TEX_HEIGHT +
		ED_CHAR_TEXY_BORDER;
	float texBottom = texTop + ED_CHAR_TEX_HEIGHT - 2.0f * ED_CHAR_TEXY_BORDER;

	float screenLeft = xPos * ED_CHAR_WIDTH / aspectRatio +
		ED_CHAR_X_OFFSET + ED_CHAR_X_BORDER / aspectRatio + xAdjust; // Adjustment for the cursor character
	float screenRight = screenLeft + ED_CHAR_WIDTH / aspectRatio - 2.0f * ED_CHAR_X_BORDER / aspectRatio;
	float screenTop = yPos * ED_CHAR_HEIGHT + ED_CHAR_Y_OFFSET + ED_CHAR_Y_BORDER;
	float screenBottom = screenTop + ED_CHAR_HEIGHT - 2.0f * ED_CHAR_Y_BORDER;

	glTexCoord2f(texLeft, 1.0f - texBottom);
	glVertex3f(screenLeft, -screenBottom, 0.5f);
	glTexCoord2f(texRight, 1.0f - texBottom);
	glVertex3f(screenRight, -screenBottom, 0.5f);
	glTexCoord2f(texRight, 1.0f - texTop);
	glVertex3f(screenRight, -screenTop, 0.5f);
	glTexCoord2f(texLeft, 1.0f - texTop);
	glVertex3f(screenLeft, -screenTop, 0.5f);
}

int Editor::getLineLength(int line)
{
	if (line < 0) return 0;
	if (line >= numLines) return 0;
	int lineLength = ED_MAX_LINE_LENGTH;
	while (lineLength > 0 && text[line][lineLength-1] == ' ')
	{
		lineLength--;
	}
	return lineLength;
}

void Editor::setSnapshot(const char *filename)
{
	strcpy_s(fileHistory[nextFileHistory], ED_MAX_FILENAME_LENGTH, filename);
	if (firstFileHistory < 0) firstFileHistory = nextFileHistory;
	lastFileHistory = nextFileHistory; // return to this snapshot on undo()
	nextFileHistory = (nextFileHistory+1) % ED_MAX_FILE_HISTORY;
	if (nextFileHistory == firstFileHistory)
	{
		// buffer overflow, remove last entry
		firstFileHistory = (firstFileHistory+1) % ED_MAX_FILE_HISTORY;
	}
}

void Editor::undo()
{
	char dummyError[MAX_ERROR_LENGTH+1];
	if (lastFileHistory < 0) return;

	// should never throw an error. Hopefully.
	if (loadText(fileHistory[lastFileHistory], dummyError) != 0)
	{
		setErrorText(dummyError);
	}

	// Go to previous file history if applicable
	if (lastFileHistory != firstFileHistory)
	{
		lastFileHistory = (lastFileHistory+ED_MAX_FILE_HISTORY-1) % ED_MAX_FILE_HISTORY;
	}
}

void Editor::redo()
{
	char dummyError[MAX_ERROR_LENGTH+1];
	if (firstFileHistory < 0) return;
	if (lastFileHistory < 0)
	{
		lastFileHistory = firstFileHistory;
	}

	int prev = (lastFileHistory + 1) % ED_MAX_FILE_HISTORY;
	if (prev == nextFileHistory) return;

	if (loadText(fileHistory[prev], dummyError) != 0)
	{
		setErrorText(dummyError);
	}
	lastFileHistory = prev;
}

void Editor::deleteLine(void)
{
	// Change --> show
	textDisplayAlpha = 1.0f;
	isTextFading = false;
	// Copy all following lines to this one
	numLines--;
	for (int line = cursorPos[1]; line < numLines; line++)
	{
		for (int k = 0; k < ED_MAX_LINE_LENGTH+1; k++)
		{
			text[line][k] = text[line+1][k];
		}
	}
	text[numLines][0] = 0; // end char?
	updateIndentation(); // may have changed after deleteing { or }
}

void Editor::controlCharacter(WPARAM vKey)
{
	switch (vKey)
	{
	case VK_RETURN:
		// Change --> show
		textDisplayAlpha = 1.0f;
		isTextFading = false;
		if (numLines < ED_MAX_NUM_LINES)
		{
			// Copy all the following lines afront (ugh...)
			for (int line = numLines; line > cursorPos[1]; line--)
			{
				for (int k = 0; k < ED_MAX_LINE_LENGTH+1; k++)
				{
					text[line][k] = text[line-1][k];
				}
			}
			numLines++;
			text[numLines][0] = 0;

			// delete everything after the cursor pos
			for (int k = cursorPos[0]; k < ED_MAX_LINE_LENGTH; k++)
			{
				text[cursorPos[1]][k] = ' ';
			}
			// delete everything before the cursor pos in next line
			for (int k = cursorPos[0]; k < ED_MAX_LINE_LENGTH; k++)
			{
				text[cursorPos[1]+1][k - cursorPos[0]] =
					text[cursorPos[1]+1][k];
			}
			for (int k = ED_MAX_LINE_LENGTH - cursorPos[0]; k < ED_MAX_LINE_LENGTH; k++)
			{
				text[cursorPos[1]+1][k] = ' ';
			}
			cursorPos[1]++;
			cursorPos[0] = 0;
			updateIndentation();
		}
		break;

	case VK_BACK:
		// Change --> show
		textDisplayAlpha = 1.0f;
		isTextFading = false;
		if (cursorPos[0] > 0)
		{
			for (int k = cursorPos[0]; k < ED_MAX_LINE_LENGTH; k++)
			{
				text[cursorPos[1]][k-1] = text[cursorPos[1]][k];
			}
			text[cursorPos[1]][ED_MAX_LINE_LENGTH-1] = ' ';
			cursorPos[0]--;
		}
		else // This is the only way to delete a line
		{
			if (cursorPos[1] > 0)
			{
				int prevLineLength = getLineLength(cursorPos[1]-1);
				int lineLength = getLineLength(cursorPos[1]);
				if (prevLineLength + lineLength <= ED_MAX_LINE_LENGTH)
				{
					// Copy line to above one
					for (int k = prevLineLength; k < ED_MAX_LINE_LENGTH; k++)
					{
						text[cursorPos[1]-1][k] =
							text[cursorPos[1]][k - prevLineLength];
					}
					// Copy all other lines
					for (int line = cursorPos[1]; line < numLines - 1; line++)
					{
						for (int k = 0; k < ED_MAX_LINE_LENGTH+1; k++)
						{
							text[line][k] = text[line+1][k];
						}
					}
					numLines--;
					text[numLines][0] = 0;
					cursorPos[1]--;
					cursorPos[0] = prevLineLength;
				}
			}
			updateIndentation();
		}
		break;

	case VK_DELETE:
		// Change --> show
		textDisplayAlpha = 1.0f;
		isTextFading = false;
		if (cursorPos[0] < ED_MAX_LINE_LENGTH)
		{
			for (int k = cursorPos[0] + 1; k < ED_MAX_LINE_LENGTH; k++)
			{
				text[cursorPos[1]][k-1] = text[cursorPos[1]][k];
			}
			text[cursorPos[1]][ED_MAX_LINE_LENGTH-1] = ' ';
		}
		break;
	}

	// When putting a control character, the cursor want position is resetted
	cursorWantX = cursorPos[0];
}

void Editor::putCharacter(WPARAM vKey)
{
	// Change --> show
	textDisplayAlpha = 1.0f;
	isTextFading = false;

	// Check whether there is space in the line left:
	if (text[cursorPos[1]][ED_MAX_LINE_LENGTH - 1] != ' ')
	{
		return;
	}

	// Only do stuff that can be displayed!
	if (charIPos[vKey][1] > 20 && vKey != ' ') return;

	// Do not insert spaces at the beginning of a line
	if (cursorPos[0] == 0 && vKey == ' ') return;

	// only enter if there is space left:
	if (cursorPos[0] < ED_MAX_LINE_LENGTH)
	{
		// move everything to the right:
		for (int i = ED_MAX_LINE_LENGTH - 1; i > cursorPos[0]; i--)
		{
			text[cursorPos[1]][i] = text[cursorPos[1]][i-1];
		}
		text[cursorPos[1]][cursorPos[0]] = vKey;
		cursorPos[0]++;
		if (cursorPos[0] > ED_MAX_LINE_LENGTH) cursorPos[0] = ED_MAX_LINE_LENGTH;

		// When putting a character, the cursor want position is resetted
		cursorWantX = cursorPos[0];

		// some magic if we entered '{'
		if (vKey == '{')
		{
			controlCharacter(VK_RETURN);
			moveCursor(VK_END);
			controlCharacter(VK_RETURN);
			putCharacter('}');
			moveCursor(VK_LEFT);
			moveCursor(VK_UP);
			updateIndentation();
		}
	}
}

void Editor::moveCursor(WPARAM vKey)
{
	// Change --> show
	textDisplayAlpha = 1.0f;
	isTextFading = false;

	switch (vKey)
	{
	case VK_LEFT:
		cursorPos[0]--;
		break;

	case VK_RIGHT:
		cursorPos[0]++;
		break;

	case VK_UP:
		cursorPos[1]--;
		cursorPos[0] = cursorWantX;
		break;

	case VK_DOWN:
		cursorPos[1]++;
		cursorPos[0] = cursorWantX;
		break;

	case VK_PRIOR:
		cursorPos[1] -= ED_PAGE_LENGTH;
		cursorPos[0] = cursorWantX;
		break;

	case VK_NEXT:
		cursorPos[1] += ED_PAGE_LENGTH;
		cursorPos[0] = cursorWantX;
		break;

	case VK_HOME:
		cursorPos[0] = 0;
		break;

	case VK_END:
		cursorPos[0] = ED_MAX_LINE_LENGTH;
		break;
	}

	// Make sure that the cursor is within range
	if (cursorPos[1] < 0) cursorPos[1] = 0;
	if (cursorPos[1] >= numLines) cursorPos[1] = numLines - 1;
	if (cursorPos[0] < 0) cursorPos[0] = 0;
	int lineLength = getLineLength(cursorPos[1]);
	if (cursorPos[0] > lineLength) cursorPos[0] = lineLength;

	// update the location where the cursor wants to be
	if (vKey == VK_LEFT || vKey == VK_RIGHT ||
		vKey == VK_HOME || vKey == VK_END)
	{
		cursorWantX = cursorPos[0];
	}
}

int Editor::loadText(const char *filename, char *errorText)
{
	// Load textbuffer
	FILE *fid;
	if (fopen_s(&fid, filename, "rb"))
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH, "IO Error: Could not open '%s'.",
				  filename);
		return -1;
	}

	int bufferSize = fread(textBuffer, 1, ED_MAX_FILESIZE, fid);
	if (bufferSize == ED_MAX_FILESIZE)
	{
		sprintf_s(errorText, MAX_ERROR_LENGTH,
				  "Editor Error: %s is too large.", filename);
		return -1;
	}

	fclose(fid);

	// Clear the buffer
	clear();
	// Reset the number of lines to (was set to 1 by clear())
	numLines = 0;

	// Basic parsing of file into buffer
	boolean isFinished = false;
	int bufferPos = 0;
	while (!isFinished)
	{
		if (numLines >= ED_MAX_NUM_LINES)
		{
			sprintf_s(errorText, MAX_ERROR_LENGTH,
				      "Editor error: File %s has too many lines.", filename);
			clear();
			return -1;
		}

		boolean lineEnded = false;
		for (int i = 0; i < ED_MAX_LINE_LENGTH; i++)
		{
			// ignore CR!
			while (bufferPos < bufferSize && textBuffer[bufferPos] == '\r')
			{
				bufferPos++;
			}

			// Only do stuff that can be displayed!
			if (charIPos[textBuffer[bufferPos]][1] > 20 &&
				textBuffer[bufferPos] != ' ' &&
				textBuffer[bufferPos] != '\n' &&
				textBuffer[bufferPos] != 0)
			{
				sprintf_s(errorText, MAX_ERROR_LENGTH,
					      "Editor error: Can not display '%c' in %s.",
						  textBuffer[bufferPos], filename);
				clear();
				return -1;
			}

			if (lineEnded)
			{
				text[numLines][i] = ' ';
			}
			else
			{
				if (bufferPos >= bufferSize)
				{
					lineEnded = true;
					isFinished = true;
				}
				else
				{
					// Check if the line ends on this char
					if (textBuffer[bufferPos] == '\n')
					{
						lineEnded = true;
						bufferPos++;
						// Scan forward until we reached some text
						while (bufferPos < bufferSize &&
							   (textBuffer[bufferPos] == ' ' ||
							    textBuffer[bufferPos] == '\r' ||
							    textBuffer[bufferPos] == '\t'))
						{
							bufferPos++;
						}

						// A newline is also written as a space
						text[numLines][i] = ' ';
					}
					else
					{
						text[numLines][i] = textBuffer[bufferPos];
						bufferPos++;
						if (bufferPos >= bufferSize)
						{
							lineEnded = true;
							isFinished = true;
						}
					}
				}
			}
		}

		// Line should have eneded here here:
		if (!lineEnded)
		{
			while ((textBuffer[bufferPos] == '\r' || textBuffer[bufferPos] == '\n') &&
				   bufferPos < bufferSize)
			{
				lineEnded = true;
				bufferPos++;
			}
			if (bufferPos >= bufferSize)
			{
				isFinished = true;
			}
			
			if (!lineEnded)
			{
				sprintf_s(errorText, MAX_ERROR_LENGTH, 
						  "Editor error: Line %d in %s too long.",
						  numLines, filename);
				clear();
				return -1;
			}
		}

		text[numLines][ED_MAX_LINE_LENGTH] = '\n';
		numLines++;
	}
	
	text[numLines][0] = 0; // Terminate text with a zero.
	
	// If the file was empty, clear everything
	if (numLines == 0)
	{
		clear();
	}

	// Create the indentation
	updateIndentation();

	return 0;
}

// Update the indentation used for rendering
void Editor::updateIndentation(void)
{
	int indent = 0;
	bool oldLineEnded = true;
	for (int line = 0; line < numLines; line++)
	{
		bool lineEnded = true;

		// look for indentation left
		for (int k = 0; k < ED_MAX_LINE_LENGTH; k++)
		{
			if (text[line][k] == '}') indent--;
			if (text[line][k] != ' ') lineEnded = false;
		}

		indentation[line] = indent;

		// look for indentation right
		for (int k = 0; k < ED_MAX_LINE_LENGTH; k++)
		{
			if (text[line][k] == '{') indent++;
			if (text[line][k] == '{') lineEnded = true;
			if (text[line][k] == ';') lineEnded = true;
			if (text[line][k] == '}') lineEnded = true;
		}
		if (!oldLineEnded) indentation[line] += 2;
		oldLineEnded = lineEnded;
	}
}

void Editor::unshowError(void)
{
	isErrorFading = true;
}

void Editor::unshowText(void)
{
	isTextFading = true;
}

void Editor::setErrorText(char *errorText)
{
	strcpy_s((char *)displayedError, MAX_ERROR_LENGTH, errorText);
	displayedError[MAX_ERROR_LENGTH] = 0;
	isErrorFading = false;
	errorDisplayAlpha = 1.0f;
}

// Creates all the indices of locations of the font
void Editor::createFontTable(void)
{
	for (int i = 0; i < 256; i++)
	{
		charIPos[i][0] = 0;
		charIPos[i][1] = 100;
	}

	charIPos['a'][0] = 0;
	charIPos['a'][1] = 0;
	charIPos['b'][0] = 1;
	charIPos['b'][1] = 0;
	charIPos['c'][0] = 2;
	charIPos['c'][1] = 0;
	charIPos['d'][0] = 3;
	charIPos['d'][1] = 0;
	charIPos['e'][0] = 4;
	charIPos['e'][1] = 0;
	charIPos['f'][0] = 5;
	charIPos['f'][1] = 0;
	charIPos['g'][0] = 6;
	charIPos['g'][1] = 0;
	charIPos['h'][0] = 7;
	charIPos['h'][1] = 0;
	charIPos['i'][0] = 8;
	charIPos['i'][1] = 0;
	charIPos['j'][0] = 9;
	charIPos['j'][1] = 0;
	charIPos['k'][0] = 10;
	charIPos['k'][1] = 0;
	charIPos['l'][0] = 11;
	charIPos['l'][1] = 0;
	charIPos['m'][0] = 12;
	charIPos['m'][1] = 0;
	
	charIPos['n'][0] = 0;
	charIPos['n'][1] = 1;
	charIPos['o'][0] = 1;
	charIPos['o'][1] = 1;
	charIPos['p'][0] = 2;
	charIPos['p'][1] = 1;
	charIPos['q'][0] = 3;
	charIPos['q'][1] = 1;
	charIPos['r'][0] = 4;
	charIPos['r'][1] = 1;
	charIPos['s'][0] = 5;
	charIPos['s'][1] = 1;
	charIPos['t'][0] = 6;
	charIPos['t'][1] = 1;
	charIPos['u'][0] = 7;
	charIPos['u'][1] = 1;
	charIPos['v'][0] = 8;
	charIPos['v'][1] = 1;
	charIPos['w'][0] = 9;
	charIPos['w'][1] = 1;
	charIPos['x'][0] = 10;
	charIPos['x'][1] = 1;
	charIPos['y'][0] = 11;
	charIPos['y'][1] = 1;
	charIPos['z'][0] = 12;
	charIPos['z'][1] = 1;

	charIPos['A'][0] = 0;
	charIPos['A'][1] = 2;
	charIPos['B'][0] = 1;
	charIPos['B'][1] = 2;
	charIPos['C'][0] = 2;
	charIPos['C'][1] = 2;
	charIPos['D'][0] = 3;
	charIPos['D'][1] = 2;
	charIPos['E'][0] = 4;
	charIPos['E'][1] = 2;
	charIPos['F'][0] = 5;
	charIPos['F'][1] = 2;
	charIPos['G'][0] = 6;
	charIPos['G'][1] = 2;
	charIPos['H'][0] = 7;
	charIPos['H'][1] = 2;
	charIPos['I'][0] = 8;
	charIPos['I'][1] = 2;
	charIPos['J'][0] = 9;
	charIPos['J'][1] = 2;
	charIPos['K'][0] = 10;
	charIPos['K'][1] = 2;
	charIPos['L'][0] = 11;
	charIPos['L'][1] = 2;	
	charIPos['M'][0] = 12;
	charIPos['M'][1] = 2;

	charIPos['N'][0] = 0;
	charIPos['N'][1] = 3;
	charIPos['O'][0] = 1;
	charIPos['O'][1] = 3;
	charIPos['P'][0] = 2;
	charIPos['P'][1] = 3;
	charIPos['Q'][0] = 3;
	charIPos['Q'][1] = 3;
	charIPos['R'][0] = 4;
	charIPos['R'][1] = 3;
	charIPos['S'][0] = 5;
	charIPos['S'][1] = 3;
	charIPos['T'][0] = 6;
	charIPos['T'][1] = 3;
	charIPos['U'][0] = 7;
	charIPos['U'][1] = 3;
	charIPos['V'][0] = 8;
	charIPos['V'][1] = 3;
	charIPos['W'][0] = 9;
	charIPos['W'][1] = 3;
	charIPos['X'][0] = 10;
	charIPos['X'][1] = 3;
	charIPos['Y'][0] = 11;
	charIPos['Y'][1] = 3;
	charIPos['Z'][0] = 12;
	charIPos['Z'][1] = 3;

	charIPos['0'][0] = 0;
	charIPos['0'][1] = 4;
	charIPos['1'][0] = 1;
	charIPos['1'][1] = 4;
	charIPos['2'][0] = 2;
	charIPos['2'][1] = 4;
	charIPos['3'][0] = 3;
	charIPos['3'][1] = 4;
	charIPos['4'][0] = 4;
	charIPos['4'][1] = 4;
	charIPos['5'][0] = 5;
	charIPos['5'][1] = 4;
	charIPos['6'][0] = 6;
	charIPos['6'][1] = 4;
	charIPos['7'][0] = 7;
	charIPos['7'][1] = 4;
	charIPos['8'][0] = 8;
	charIPos['8'][1] = 4;
	charIPos['9'][0] = 9;
	charIPos['9'][1] = 4;
	charIPos['0'][0] = 10;
	charIPos['0'][1] = 4;
	charIPos['-'][0] = 11;
	charIPos['-'][1] = 4;
	charIPos['='][0] = 12;
	charIPos['='][1] = 4;

	charIPos['['][0] = 0;
	charIPos['['][1] = 5;
	charIPos[']'][0] = 1;
	charIPos[']'][1] = 5;
	charIPos[';'][0] = 2;
	charIPos[';'][1] = 5;
	charIPos['\''][0] = 3;
	charIPos['\''][1] = 5;
	charIPos['\\'][0] = 4;
	charIPos['\\'][1] = 5;
	charIPos[','][0] = 5;
	charIPos[','][1] = 5;
	charIPos['.'][0] = 6;
	charIPos['.'][1] = 5;
	charIPos['/'][0] = 7;
	charIPos['/'][1] = 5;

	charIPos['!'][0] = 0;
	charIPos['!'][1] = 6;
	charIPos['@'][0] = 1;
	charIPos['@'][1] = 6;
	charIPos['#'][0] = 2;
	charIPos['#'][1] = 6;
	charIPos['$'][0] = 3;
	charIPos['$'][1] = 6;
	charIPos['%'][0] = 4;
	charIPos['%'][1] = 6;
	charIPos['^'][0] = 5;
	charIPos['^'][1] = 6;
	charIPos['&'][0] = 6;
	charIPos['&'][1] = 6;
	charIPos['*'][0] = 7;
	charIPos['*'][1] = 6;
	charIPos['('][0] = 8;
	charIPos['('][1] = 6;
	charIPos[')'][0] = 9;
	charIPos[')'][1] = 6;
	charIPos['_'][0] = 10;
	charIPos['_'][1] = 6;
	charIPos['+'][0] = 11;
	charIPos['+'][1] = 6;
	charIPos['{'][0] = 12;
	charIPos['{'][1] = 6;

	charIPos['}'][0] = 0;
	charIPos['}'][1] = 7;
	charIPos[':'][0] = 1;
	charIPos[':'][1] = 7;
	charIPos['"'][0] = 2;
	charIPos['"'][1] = 7;
	charIPos['|'][0] = 3;
	charIPos['|'][1] = 7;
	charIPos['<'][0] = 4;
	charIPos['<'][1] = 7;
	charIPos['>'][0] = 5;
	charIPos['>'][1] = 7;
	charIPos['?'][0] = 6;
	charIPos['?'][1] = 7;
}