#include "StdAfx.h"
#include "ScreenBorders.h"
#include "Configuration.h"

ScreenBorders::ScreenBorders(void)
{
	// Default values for screen borders
	xBorder[0][0] = -0.9f;
	xBorder[0][1] = -0.4f;
	xBorder[1][0] = -0.2f;
	xBorder[1][1] = 0.3f;
	xBorder[2][0] = 0.4f;
	xBorder[2][1] = 0.9f;

	yBorder[0][0] = -0.8f;
	yBorder[0][1] = 0.6f;
	yBorder[1][0] = -0.6f;
	yBorder[1][1] = 0.8f;
	yBorder[2][0] = -0.8f;
	yBorder[2][1] = 0.6f;
}


ScreenBorders::~ScreenBorders(void)
{
}

void ScreenBorders::drawBorders(TextureManager *tex, HWND mainWnd)
{
	// set up matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH + 1];
	if (tex->getTextureID("black", &texID, errorString))
	{
		MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
		return;
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	
	glBegin(GL_QUADS);
	drawQuad(-1.0f, -1.0f, xBorder[0][0], 1.0f);
	drawQuad(xBorder[0][0], -1.0f, xBorder[0][1], yBorder[0][0]);
	drawQuad(xBorder[0][0], yBorder[0][1], xBorder[0][1], 1.0f);
	drawQuad(xBorder[0][1], -1.0f, xBorder[1][0], 1.0f);
	drawQuad(xBorder[1][0], -1.0f, xBorder[1][1], yBorder[1][0]);
	drawQuad(xBorder[1][0], yBorder[1][1], xBorder[1][1], 1.0f);
	drawQuad(xBorder[1][1], -1.0f, xBorder[2][0], 1.0f);
	drawQuad(xBorder[2][0], -1.0f, xBorder[2][1], yBorder[2][0]);
	drawQuad(xBorder[2][0], yBorder[2][1], xBorder[2][1], 1.0f);
	drawQuad(xBorder[2][1], -1.0f, 1.0f, 1.0f);
	glEnd();
}

void ScreenBorders::drawQuad(float left, float bottom, float right, float top)
{
	glVertex3f(left, top, 0.99f);
	glVertex3f(right, top, 0.99f);
	glVertex3f(right, bottom, 0.99f);
	glVertex3f(left, bottom, 0.99f);
}