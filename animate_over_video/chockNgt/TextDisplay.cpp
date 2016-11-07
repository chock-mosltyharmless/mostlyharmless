#include "StdAfx.h"
#include "TextDisplay.h"
#include "Configuration.h"
#include "glext.h"

TextDisplay::TextDisplay(void) {
	clear();
}

TextDisplay::~TextDisplay(void) {
}

void TextDisplay::clear(void) {
}

int TextDisplay::init(TextureManager *textureMgr, char *errorText) {
	textureManager = textureMgr;
	createFontTable();
	return 0;
}

void TextDisplay::ShowText(float xp, float yp, const char *text) {
	// So far I just render from the top
	// There is no room for error here!
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH + 1];
	textureManager->getTextureID("font_512.png", &texID, errorString);
	glBindTexture(GL_TEXTURE_2D, texID);

	// For now I am dumb.
	// TODO: I only draw in a sensible range!!!!
	//       And fade out whatever...
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	// Draw text
	float yDisplay = yp;
	float alpha = 1.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    float indent = 0.0f;
    int length = strlen(text);
	for (int x = 0; x < length; x++) {
		if (text[x] != ' ') {
			drawChar((float)x + indent, yDisplay, text[x]);
		}
	}

	glEnd();
}

void TextDisplay::drawChar(float xPos, float yPos, unsigned char textBit)
{
	// Special handling for the cursor character
	float xAdjust = 0.0f;

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

	glTexCoord2f(texLeft, texBottom);
	glVertex3f(screenLeft, -screenBottom, 0.5f);
	glTexCoord2f(texRight, texBottom);
	glVertex3f(screenRight, -screenBottom, 0.5f);
	glTexCoord2f(texRight, texTop);
	glVertex3f(screenRight, -screenTop, 0.5f);
	glTexCoord2f(texLeft, texTop);
	glVertex3f(screenLeft, -screenTop, 0.5f);
}

// Creates all the indices of locations of the font
void TextDisplay::createFontTable(void)
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