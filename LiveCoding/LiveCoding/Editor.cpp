#include "StdAfx.h"
#include "Editor.h"
#include "Configuration.h"
#include "GLNames.h"
#include "glext.h"

Editor::Editor(void)
{
	clear();
}

Editor::~Editor(void)
{
}

void Editor::clear(void)
{
	numLines = 0;
	for (int k = 0; k < ED_MAX_NUM_LINES; k++)
	{
		text[k][0] = 0;
	}
}

int Editor::init(ShaderManager *shaderMgr, TextureManager *textureMgr, char *errorText)
{
	shaderManager = shaderMgr;
	textureManager = textureMgr;
	createFontTable();
	return 0;
}

void Editor::render(void)
{
	// So far I just render from the top
	// There is no room for error here!
	GLuint texID;
	GLuint progID;
	char errorString[MAX_ERROR_LENGTH + 1];
	textureManager->getTextureID("font_512.tga", &texID, errorString);
	glBindTexture(0, texID);
	glColor4f(0.8f, 0.93f, 1.0f, 1.0f);
	shaderManager->getProgramID("SimpleTexture.gprg", &progID, errorString);
	glUseProgram(progID);	

	// For now I am dumb.
	// TODO: I only draw in a sensible range!!!!
	//       And fade out whatever...
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glBegin(GL_QUADS);
	for (int y = 0; y < 100; y++)
	{
		for (int x = 0; x < ED_MAX_LINE_LENGTH; x++)
		{
			if (text[y][x] != ' ')
			{
				drawChar(x, y, text[y][x]);
			}
		}
	}
	glEnd();
}

void Editor::drawChar(int xPos, int yPos, unsigned char textBit)
{
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

	float screenLeft = xPos * ED_CHAR_WIDTH + ED_CHAR_X_OFFSET + ED_CHAR_X_BORDER;
	float screenRight = screenLeft + ED_CHAR_WIDTH - 2.0f * ED_CHAR_X_BORDER;
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

	// Basic parsing of file into buffer
	boolean isFinished = false;
	int bufferPos = 0;
	while (!isFinished)
	{
		if (numLines >= ED_MAX_NUM_LINES)
		{
			sprintf_s(errorText, MAX_ERROR_LENGTH,
				      "Editor error: File %s has too many lines.", filename);
			return -1;
		}

		boolean lineEnded = false;
		for (int i = 0; i < ED_MAX_LINE_LENGTH; i++)
		{
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
					if (textBuffer[bufferPos] == '\n' ||
						textBuffer[bufferPos] == '\r')
					{
						lineEnded = true;
						bufferPos++;
						// Scan forward until we reached some text
						while (bufferPos < bufferSize &&
							   (textBuffer[bufferPos] == ' ' ||
							    textBuffer[bufferPos] == '\t' ||
								textBuffer[bufferPos] == '\r' ||
								textBuffer[bufferPos] == '\n'))
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
					}
				}
			}
		}

		// Line ends here:
		if (!lineEnded)
		{
			sprintf_s(errorText, MAX_ERROR_LENGTH, 
					  "Editor error: Line %d in %s too long.",
					  numLines, filename);
			return -1;
		}

		text[numLines][ED_MAX_LINE_LENGTH] = '\n';
		numLines++;
	}
	
	text[numLines][0] = 0; // Terminate text with a zero.
	
	return 0;
}

// Creates all the indices of locations of the font
void Editor::createFontTable(void)
{
	for (int i = 0; i < 256; i++)
	{
		charIPos[i][0] = 0;
		charIPos[i][1] = 10;
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