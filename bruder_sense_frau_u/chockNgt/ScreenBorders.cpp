#include "StdAfx.h"
#include "ScreenBorders.h"
#include "Configuration.h"
#include "Parameter.h"

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

void ScreenBorders::drawBorders(TextureManager *tex, HWND mainWnd, bool showBlue, float opacity)
{
	// set up matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
#if 0	
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
#endif

	// Get values for screen borders
	xBorder[0][0] = params.getParam(2, 0.2f) * 0.5f - 1.0f;
	xBorder[0][1] = xBorder[0][0] + params.getParam(3, 0.7f) * 1.5f / 3.0f + 0.5f / 3.0f;
	xBorder[1][0] = -7.0f/12.0f + params.getParam(4, 0.7f) * 0.5f;
	xBorder[1][1] = xBorder[1][0] + params.getParam(5, 0.7f) * 1.5f / 3.0f + 0.5f / 3.0f;
	xBorder[2][0] = 1.0f / 12.0f + params.getParam(6, 0.7f) * 0.5f;
	xBorder[2][1] = xBorder[2][0] + params.getParam(8, 0.7f) * 1.5f / 3.0f + 0.5f / 3.0f;
	yBorder[0][0] = params.getParam(14, 0.2f) - 1.0f;
	yBorder[0][1] = 1.0f - params.getParam(15, 0.2f);
	yBorder[1][0] = params.getParam(16, 0.2f) - 1.0f;
	yBorder[1][1] = 1.0f - params.getParam(17, 0.2f);
	yBorder[2][0] = params.getParam(18, 0.2f) - 1.0f;
	yBorder[2][1] = 1.0f - params.getParam(19, 0.2f);


	GLuint offscreenTexture;
	char errorString[MAX_ERROR_LENGTH + 1];
	if (tex->getTextureID("renderTarget", &offscreenTexture, errorString))
	{
		MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
		return;
	}
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, X_OFFSCREEN, Y_OFFSCREEN);   //Copy back buffer to texture

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// Draw blue borders if applicable
	if (showBlue)
	{
		GLuint texID;
		if (tex->getTextureID("Blue_top_tall_text.tga", &texID, errorString))
		{
			MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
			return;
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		glBegin(GL_QUADS);
		for (int i = 0; i < 3; i++)
		{
			float height = (yBorder[i][1] - yBorder[i][0]) * 13.3f / 170.0f;
			drawQuad(xBorder[i][0], yBorder[i][1] - height, xBorder[i][1], yBorder[i][1], 0.0f, 1.0f);
			yBorder[i][1] -= height;
		}
		glEnd();
		if (tex->getTextureID("Blue_bottom_tall_text.tga", &texID, errorString))
		{
			MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
			return;
		}
		glBindTexture(GL_TEXTURE_2D, texID);
		glBegin(GL_QUADS);
		for (int i = 0; i < 3; i++)
		{
			float height = (yBorder[i][1] - yBorder[i][0]) * 8.3f / (170.0f-13.3f);
			drawQuad(xBorder[i][0], yBorder[i][0], xBorder[i][1], yBorder[i][0] + height, 0.0f, 1.0f);
			yBorder[i][0] += height;
		}
		glEnd();
	}

	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glBegin(GL_QUADS);
	float dimmer = params.getParam(22, 0.0f);
	float redenner = params.getParam(13, 0.0f);
	glColor4f((1.0f - dimmer) * opacity, (1.0f - dimmer - redenner * 0.7f) * opacity,
		      (1.0f - dimmer - redenner * 0.75f) * opacity, 1.0f);
	drawQuad(xBorder[0][0], yBorder[0][0], xBorder[0][1], yBorder[0][1], 0.0f, 0.33f);
	drawQuad(xBorder[1][0], yBorder[1][0], xBorder[1][1], yBorder[1][1], 0.34f, 0.66f);
	drawQuad(xBorder[2][0], yBorder[2][0], xBorder[2][1], yBorder[2][1], 0.67f, 1.0f);
	glEnd();
}

void ScreenBorders::drawQuad(float left, float bottom, float right, float top,
	                         float tx1, float tx2)
{
	glTexCoord2f(tx1, 1.0f);
	glVertex3f(left, top, 0.99f);
	glTexCoord2f(tx2, 1.0f);
	glVertex3f(right, top, 0.99f);
	glTexCoord2f(tx2, 0.0f);
	glVertex3f(right, bottom, 0.99f);
	glTexCoord2f(tx1, 0.0f);
	glVertex3f(left, bottom, 0.99f);
}