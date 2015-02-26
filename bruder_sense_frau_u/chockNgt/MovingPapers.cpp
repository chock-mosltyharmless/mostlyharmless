#include "StdAfx.h"
#include "MovingPapers.h"
#include "Configuration.h"

const char *MovingPapers::texNames[NUM_PAPER_TEXTURES] =
{
	"1.tga",
	"2.tga",
	"3.tga",
	"4.tga",
	"5.tga",
	"6.tga",
	"7.tga",
	"8.tga",
	"9.tga",
};

MovingPapers::MovingPapers()
{
}


MovingPapers::~MovingPapers(void)
{
}

void MovingPapers::draw(float time, HWND mainWnd, TextureManager *texManag,
	                    bool drawVideo)
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
	float paperTime = time / (float)PAPER_PERIOD;
	int timeIndex = (int)paperTime;
	float innerTime = paperTime - (float)timeIndex;

	float xBorder[3][2] = {{-1.0f, -0.3333f}, {-0.3333f, 0.3333f}, {0.3333f, 1.0f}};
	float yBorder[3][2] = {{-1.0f, 1.0f}, {-1.0f, 1.0f}, {-1.0f, 1.0f}};

	// Draw the four relevant papers.
	for (int i = 0; i < 4; i++) // right to left
	{
		int paperID = (i + timeIndex) % NUM_PAPER_TEXTURES;

		int retVal = -1;
		if (drawVideo) retVal = texManag->getVideoID("2-old.avi", &texID, errorString, (int)(time * 30.0f));
		else retVal = texManag->getTextureID(texNames[paperID], &texID, errorString);
		if (retVal != 0)
		{
			MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
			return;
		}
		glBindTexture(GL_TEXTURE_2D, texID);

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		float paperWidth = xBorder[0][1] - xBorder[0][0];
		float paperHeight = yBorder[0][1] - yBorder[0][0];

		float xPos[5] = {-1.0f - paperWidth, xBorder[0][0], xBorder[1][0], xBorder[2][0], 1.0f};
		float yPos[5] = {yBorder[0][0], yBorder[0][0], yBorder[1][0], yBorder[2][0], yBorder[2][0]};
		float t = innerTime / (float)PAPER_MOVEMENT_TIME;
		t -= i * (float)PAPER_MOVEMENT_DELTA;
		if (t > 1.0f) t = 1.0f;
		if (t < 0.0f) t = 0.0f;
		float tx = 0.5f - 0.5f * cos(t * 3.1415926f);
		float paperLeft = tx * xPos[4 - i] + (1.0f - tx) * xPos[3 - i];
		// steeper movement for y
		float ty = 0.5f - 0.5f * cos(tx * 3.1415926f);
		float paperBottom = ty * yPos[4 - i] + (1.0f - ty) * yPos[3 - i];

		if (drawVideo)
		{
			int vidPos = (300000000 - timeIndex - i) % 3;
			drawQuad(paperLeft, paperBottom, paperLeft + paperWidth, paperBottom + paperHeight,
					(float)vidPos / 3.0f, (float)(vidPos+1) / 3.0f);
		}
		else
		{
			drawQuad(paperLeft, paperBottom, paperLeft + paperWidth, paperBottom + paperHeight,
					0.0f, 1.0f);
		}
		glEnd();
	}
}

void MovingPapers::drawQuad(float left, float bottom, float right, float top,
	                        float leftU, float rightU)
{
	glTexCoord2f(leftU, 1.0f);
	glVertex3f(left, top, 0.99f);
	glTexCoord2f(rightU, 1.0f);
	glVertex3f(right, top, 0.99f);
	glTexCoord2f(rightU, 0.0f);
	glVertex3f(right, bottom, 0.99f);
	glTexCoord2f(leftU, 0.0f);
	glVertex3f(left, bottom, 0.99f);
}