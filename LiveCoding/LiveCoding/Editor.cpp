#include "StdAfx.h"
#include "Editor.h"
#include "Configuration.h"


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
	return 0;
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
