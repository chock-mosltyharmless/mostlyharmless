#include "StdAfx.h"
#include "MovingPapers.h"
#include "Configuration.h"

#ifndef PI
#define PI 3.1415926f
#endif

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
	init();
}

MovingPapers::~MovingPapers(void)
{
}

void MovingPapers::init()
{
	time = 0.0;

	for (int paperIdx = 0; paperIdx < NUM_PAPERS; paperIdx++)
	{
		paper[paperIdx].pos[0] = -1.0f - 2.0f/3.0f * (paperIdx + 1);
		paper[paperIdx].pos[1] = -1.0f;
		for (int tileY = 0; tileY < PAPER_Y_TILING; tileY++)
		{
			for (int tileX = 0; tileX < PAPER_X_TILING; tileX++)
			{
				paper[paperIdx].snippet[tileY][tileX].attached = true;
			}
		}

		for (int py = 0; py <= PAPER_Y_TILING; py++)
		{
			for (int px = 0; px <= PAPER_X_TILING; px++)
			{
				float i = (float)(py + paperIdx * PAPER_Y_TILING + px * PAPER_X_TILING);
				float displ0 = 0.5f + 1.0f * cos(i * 7602.2234f + px * 2517.3234f);
				float displ1 = 0.5f + 1.0f * cos(i * 3455.2523f + px * 8435.7234f);
				if (py == 0 || py == PAPER_Y_TILING || px == 0 || px == PAPER_X_TILING)
				{
					displ0 = displ1 = 0;
				}

				paper[paperIdx].texPos[py][px][0] = (float)px / (PAPER_X_TILING) + 0.5f * displ0 / PAPER_X_TILING;
				paper[paperIdx].texPos[py][px][1] = (float)py / (PAPER_Y_TILING) + 0.5f * displ1 / PAPER_Y_TILING;

				paper[paperIdx].tilePos[py][px][0] = displ0 / PAPER_X_TILING * 2.0f / 3.0f * 0.5f; // Hmm.. Dunno.
				paper[paperIdx].tilePos[py][px][1] = displ1 / PAPER_Y_TILING * 2.0f * 0.5f;
				paper[paperIdx].tilePos[py][px][2] = 0.0f;
			}
		}
	}
}

void MovingPapers::update(float deltaTime)
{
	time += deltaTime;

	float paperTime = time / (float)PAPER_PERIOD;
	int timeIndex = (int)paperTime;
	float innerTime = paperTime - (float)timeIndex;
	float innerMove = innerTime / PAPER_MOVEMENT_TIME;
	if (innerMove > 1.0f) innerMove = 1.0f;
	float t = 0.5f - 0.5f * cos(innerMove * 3.1415926f);
	t = 0.5f - 0.5f * cos(t * 3.1415926f);

	for (int paperIdx = 0; paperIdx < NUM_PAPERS; paperIdx++)
	{
		int posIndex = timeIndex - paperIdx - 1;
		while (posIndex > 3) posIndex -= NUM_PAPERS;
		paper[paperIdx].pos[0] = t * 2.0f / 3.0f - 1.0f + 2.0f/3.0f * posIndex;

		for (int py = 0; py < PAPER_Y_TILING; py++)
		{
			for (int px = 0; px < PAPER_X_TILING; px++)
			{
				if (paper[paperIdx].snippet[py][px].attached)
				{
					paper[paperIdx].snippet[py][px].pos[0] =
						paper[paperIdx].pos[0] + ((float)px + 0.5f) / PAPER_X_TILING * 2.0f / 3.0f;
					paper[paperIdx].snippet[py][px].pos[1] =
						paper[paperIdx].pos[1] + ((float)py + 0.5f) / PAPER_Y_TILING * 2.0f;
					paper[paperIdx].snippet[py][px].pos[2] = 0.99f;

					paper[paperIdx].snippet[py][px].rpy[0] = 0.0f;
					paper[paperIdx].snippet[py][px].rpy[1] = 0.0f;
					paper[paperIdx].snippet[py][px].rpy[2] = 0.0f;

					// Speed should be derived from possible movement...
					paper[paperIdx].snippet[py][px].speed[0] = 0.0f;
					paper[paperIdx].snippet[py][px].speed[1] = 0.0f;
					paper[paperIdx].snippet[py][px].speed[2] = 0.0f;
				}
				else
				{
					// Do a little bit of rotation
					for (int dim = 0; dim < 3; dim++)
					{
						paper[paperIdx].snippet[py][px].rpy[dim] += deltaTime*0.9f;
						while (paper[paperIdx].snippet[py][px].rpy[dim] > PI*2.0f) paper[paperIdx].snippet[py][px].rpy[dim] -= PI*2.0f;
					}

					// Move according to speed
					for (int dim = 0; dim < 3; dim++)
					{
						paper[paperIdx].snippet[py][px].pos[dim] += paper[paperIdx].snippet[py][px].speed[dim] * deltaTime;
					}

					// Gravitate
					paper[paperIdx].snippet[py][px].speed[1] -= 2.f * deltaTime;

					// Remove normal component
					float normal[3] = {0.0f, 0.0f, 1.0f};
					rotate(normal, paper[paperIdx].snippet[py][px].rpy);
					float scalProd = 0.0f;
					for (int dim = 0; dim < 3; dim++)
					{
						scalProd += normal[dim] * paper[paperIdx].snippet[py][px].speed[dim];
					}
		
					// Remove it completely
					for (int dim = 0; dim < 3; dim++)
					{
						paper[paperIdx].snippet[py][px].speed[dim] -= scalProd * normal[dim];
					}

					// Slow down due to air resistance
					for (int dim = 0; dim < 3; dim++)
					{
						paper[paperIdx].snippet[py][px].speed[dim] *= (float)exp(deltaTime * -8.f);
					}
				}

				// un-attach randomly
				int timePos = ((py * 2 + px * 5890 + paperIdx * 2391445) % 472 + 20357) % 157 + py * 20;
				if (time > (float)timePos * 0.1f) paper[paperIdx].snippet[py][px].attached = false;
			}
		}
	}
}

void MovingPapers::draw(HWND mainWnd, TextureManager *texManag, bool drawVideo)
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

	for (int pass = 0; pass < 2; pass++)
	{
		for (int paperID = 0; paperID < NUM_PAPERS; paperID++)
		{
			int retVal = -1;
			if (drawVideo) retVal = texManag->getVideoID("2-old.avi", &texID, errorString, (int)(time * 30.0f));
			else retVal = texManag->getTextureID(texNames[paperID], &texID, errorString);
			if (retVal != 0)
			{
				MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
				return;
			}
			glBindTexture(GL_TEXTURE_2D, texID);

			// Draw all papers
			glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#if 0
			drawQuad(paper[paperID].pos[0], paper[paperID].pos[1],
						paper[paperID].pos[0] + 2.0f / 3.0f, paper[paperID].pos[1] + 2.0f,
	  					0.0f, 1.0f);
#else
			// Corners are top-left, top-right, bottom-right, bottom-left
			float cornerPos[4][2] =
			{
				{-1.0f / 3.0f / PAPER_X_TILING, -1.0f / PAPER_Y_TILING},
				{1.0f / 3.0f / PAPER_X_TILING, -1.0f / PAPER_Y_TILING},
				{1.0f / 3.0f / PAPER_X_TILING, 1.0f / PAPER_Y_TILING},
				{-1.0f / 3.0f / PAPER_X_TILING, 1.0f / PAPER_Y_TILING}
			};
			for (int py = 0; py < PAPER_Y_TILING; py++)
			{
				for (int px = 0; px < PAPER_X_TILING; px++)
				{
					if ((pass == 0 && paper[paperID].snippet[py][px].attached) ||
						(pass == 1 && !paper[paperID].snippet[py][px].attached))
					{
						glTexCoord2f(paper[paperID].texPos[py][px][0], paper[paperID].texPos[py][px][1]);
						glVertex3f(paper[paperID].snippet[py][px].pos[0] + paper[paperID].tilePos[py][px][0] + cornerPos[0][0],
								   paper[paperID].snippet[py][px].pos[1] + paper[paperID].tilePos[py][px][1] + cornerPos[0][1],
								   0.99f);
						glTexCoord2f(paper[paperID].texPos[py][px+1][0], paper[paperID].texPos[py][px+1][1]);
						glVertex3f(paper[paperID].snippet[py][px].pos[0] + paper[paperID].tilePos[py][px+1][0] + cornerPos[1][0],
								   paper[paperID].snippet[py][px].pos[1] + paper[paperID].tilePos[py][px+1][1] + cornerPos[1][1],
								   0.99f);
						glTexCoord2f(paper[paperID].texPos[py+1][px+1][0], paper[paperID].texPos[py+1][px+1][1]);
						glVertex3f(paper[paperID].snippet[py][px].pos[0] + paper[paperID].tilePos[py+1][px+1][0] + cornerPos[2][0],
								   paper[paperID].snippet[py][px].pos[1] + paper[paperID].tilePos[py+1][px+1][1] + cornerPos[2][1],
								   0.99f);
						glTexCoord2f(paper[paperID].texPos[py+1][px][0], paper[paperID].texPos[py+1][px][1]);
						glVertex3f(paper[paperID].snippet[py][px].pos[0] + paper[paperID].tilePos[py+1][px][0] + cornerPos[3][0],
								   paper[paperID].snippet[py][px].pos[1] + paper[paperID].tilePos[py+1][px][1] + cornerPos[3][1],
								   0.99f);
					}
				}
			}
#endif
			glEnd();
		}
	}

#if 0
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
#if 0
			int vidPos = (300000000 - timeIndex - i) % 3;
			drawQuad(paperLeft, paperBottom, paperLeft + paperWidth, paperBottom + paperHeight,
					(float)vidPos / 3.0f, (float)(vidPos+1) / 3.0f);
#endif
		}
		else
		{
			drawQuad(paperLeft, paperBottom, paperLeft + paperWidth, paperBottom + paperHeight,
					0.0f, 1.0f);
		}
		glEnd();
	}
#endif
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

void MovingPapers::rotate(float pos[3], float rpy[3])
{
	float t[3];

	t[0] = pos[0];
	t[1] = cos(rpy[0]) * pos[1] - sin(rpy[0]) * pos[2];
	t[2] = sin(rpy[0]) * pos[1] + cos(rpy[0]) * pos[2];

	pos[0] = cos(rpy[1]) * t[0] - sin(rpy[1]) * t[2];
	pos[1] = t[1];
	pos[2] = sin(rpy[1]) * t[0] + cos(rpy[1]) * t[2];

	t[0] = cos(rpy[2]) * pos[0] - sin(rpy[2]) * pos[1];
	t[1] = sin(rpy[2]) * pos[0] + cos(rpy[2]) * pos[1];
	t[2] = pos[2];

	pos[0] = t[0];
	pos[1] = t[1];
	pos[2] = t[2];
}