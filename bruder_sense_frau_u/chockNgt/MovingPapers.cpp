#include "StdAfx.h"
#include "MovingPapers.h"
#include "Configuration.h"

#ifndef PI
#define PI 3.1415926f
#endif

const char *MovingPapers::texNames[NUM_PAPER_TEXTURES] =
{
	"n1.tga",
	"n2.tga",
	"n3.tga",
	"n17.tga",
	"n28.tga",
	"s1.tga",
	"s2.tga",
	"s3.tga",
	"n4.tga",
	"n5.tga",
	"n6.tga",
	"t1.tga",
	"t2.tga",
	"t3.tga",
	"n7.tga",
	"n8.tga",
	"n9.tga",
	"n10.tga",
	"s4.tga",
	"s5.tga",
	"s6.tga",
	"n11.tga",
	"n12.tga",
	"n13.tga",
	"g1.tga",
	"g2.tga",
	"g3.tga",
	"g4.tga",
	"n14.tga",
	"n15.tga",
	"n16.tga",
	"t4.tga",
	"t5.tga",
	"t6.tga",
	"n17.tga",
	"n18.tga",
	"n19.tga",
	"n20.tga",
	"s7.tga",
	"s8.tga",
	"s9.tga",
	"n21.tga",
	"n22.tga",
	"n23.tga",
	"g5.tga",
	"g6.tga",
	"g7.tga",
	"g8.tga",
	"n24.tga",
	"n25.tga",
	"n26.tga",
	"t7.tga",
	"t8.tga",
	"t9.tga",
	"n27.tga",
	"n28.tga",
	"n29.tga",
	"s10.tga",
	"s11.tga",
	"s12.tga",
	"n30.tga",
	"n31.tga",
	"n32.tga",
	"n33.tga",
	"g9.tga",
	"g10.tga",
	"g11.tga",
	"g12.tga",
	"n34.tga",
	"n11.tga",
	"n15.tga",
	"n16.tga",
	"s13.tga",
	"s14.tga",
	"s15.tga",
	"n23.tga",
	"n24.tga",
	"n25.tga",
	"t10.tga",
	"t11.tga",
	"t12.tga",
	"n5.tga",
	"n6.tga",
	"n7.tga",
	"s16.tga",
	"s17.tga",
	"s18.tga",
	"n26.tga",
	"n27.tga",
	"n28.tga",
	"g13.tga",
	"g14.tga",
	"g15.tga",
	"g16.tga",
	"n21.tga",
	"n22.tga",
	"n23.tga",
	"t13.tga",
	"t14.tga",
	"t15.tga",
	"n30.tga",
	"n32.tga",
	"n33.tga",
	"s19.tga",
	"s20.tga",
	"s21.tga",
};

MovingPapers::MovingPapers()
{
	startPaperIndex = 0;
	init(false);
}

MovingPapers::~MovingPapers(void)
{
}

void MovingPapers::init(bool startWithAll)
{
	time = 0.0;
	doDetach = false;
	doFeeding = true;
	detachingTime = 0.0f;
	this->startWithAll = startWithAll;
	startPaperIndex += 65; // THIS IS GLOBAL!!!

	for (int paperIdx = 0; paperIdx < NUM_PAPERS; paperIdx++)
	{
		paper[paperIdx].updatePos = true;
		paper[paperIdx].texIdx = paperIdx;
		paper[paperIdx].pos[0] = -1.0f - 2.0f/3.0f * (paperIdx + 1);
		if (startWithAll) paper[paperIdx].pos[0] += 2.0f;
		paper[paperIdx].pos[1] = -1.0f;
		for (int tileY = 0; tileY < PAPER_Y_TILING; tileY++)
		{
			for (int tileX = 0; tileX < PAPER_X_TILING; tileX++)
			{
				paper[paperIdx].snippet[tileY][tileX].attached = true;
				paper[paperIdx].snippet[tileY][tileX].detachedTime = 0.0f;
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

				paper[paperIdx].texPos[py][px][0] = (float)px / (PAPER_X_TILING) + 0.6f * displ0 / PAPER_X_TILING;
				paper[paperIdx].texPos[py][px][1] = (float)py / (PAPER_Y_TILING) + 0.6f * displ1 / PAPER_Y_TILING;

				paper[paperIdx].tilePos[py][px][0] = displ0 / PAPER_X_TILING * 2.0f / 3.0f * 0.6f; // Hmm.. Dunno.
				paper[paperIdx].tilePos[py][px][1] = displ1 / PAPER_Y_TILING * 2.0f * 0.6f;
				paper[paperIdx].tilePos[py][px][2] = 0.0f;
			}
		}
	}
}

void MovingPapers::update(float deltaTime, bool noMovement)
{
	time += deltaTime;
	if (doDetach) detachingTime += deltaTime;

	float paperTime = time / (float)PAPER_PERIOD;
	int timeIndex = (int)paperTime;
	float innerTime = paperTime - (float)timeIndex;
	float innerMove = innerTime / PAPER_MOVEMENT_TIME;
	if (innerMove > 1.0f) innerMove = 1.0f;
	float t = 0.5f - 0.5f * cos(innerMove * PI);
	t = 0.5f - 0.5f * cos(t * PI);

	for (int paperIdx = 0; paperIdx < NUM_PAPERS; paperIdx++)
	{
		paper[paperIdx].texIdx = paperIdx + startPaperIndex;
		int posIndex = timeIndex - paperIdx - 1;
		if (startWithAll) posIndex += 3;
		while (posIndex > NUM_PAPERS - 3)
		{
			posIndex -= NUM_PAPERS;
			paper[paperIdx].texIdx += NUM_PAPERS;
		}
		paper[paperIdx].texIdx %= NUM_PAPER_TEXTURES;

		if (!doFeeding)
		{
			if (posIndex < -1) paper[paperIdx].updatePos = false;
		}

		if (paper[paperIdx].updatePos) paper[paperIdx].pos[0] = t * 2.0f / 3.0f - 1.0f + 2.0f/3.0f * posIndex;
		else paper[paperIdx].pos[0] = -10.0f; // pretty far away...

		if (noMovement)
		{
			paper[paperIdx].pos[0] = 1.0f - paperIdx * 2.0f / 3.0f;
		}

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
					// It's now longer detached
					paper[paperIdx].snippet[py][px].detachedTime += deltaTime;

					// Do a little bit of rotation
					for (int dim = 0; dim < 3; dim++)
					{
						paper[paperIdx].snippet[py][px].rpy[dim] += deltaTime*cos(dim * 34.12380f + px * 20.1239f + py * 12.1234f + time * 0.5f);
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
				if (doDetach)
				{
					//int timePos = ((py*2  + px * 58901 + paperIdx * 2391445) % 1471 + 20357) % 157 + py * 20;
					int timePos = ((py*2  + px * 58901 + paperIdx * 2391445) % 1471 + 20357) % 127 + py * 18;
					if (detachingTime > (float)timePos * 0.075f) paper[paperIdx].snippet[py][px].attached = false;
				}
			}
		}
	}
}

void MovingPapers::draw(HWND mainWnd, TextureManager *texManag, bool useConstTexture, GLuint texID)
{
	// set up matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	char errorString[MAX_ERROR_LENGTH + 1];

	for (int pass = 0; pass < 2; pass++)
	{
		for (int paperID = 0; paperID < NUM_PAPERS; paperID++)
		{
			int retVal = -1;
			//if (drawVideo) retVal = texManag->getVideoID("2-old.avi", &texID, errorString, (int)(time * 30.0f));
			//else retVal = texManag->getTextureID(texNames[paperID], &texID, errorString);
			if (!useConstTexture) 
			{
				int texIdx = paper[paperID].texIdx;
				retVal = texManag->getTextureID(texNames[texIdx], &texID, errorString);
				if (retVal != 0)
				{
					MessageBox(mainWnd, errorString, "Texture Manager get texture ID", MB_OK);
					return;
				}
			}
			glBindTexture(GL_TEXTURE_2D, texID);

			// Draw all papers
			glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 0.92f, 1.0f);
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
					if (pass == 0 && paper[paperID].snippet[py][px].attached)
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
					else if (pass == 1 && !paper[paperID].snippet[py][px].attached)
					{
						if (paper[paperID].snippet[py][px].pos[0] < -1.5f ||
							paper[paperID].snippet[py][px].pos[0] > 1.5f ||
							paper[paperID].snippet[py][px].pos[1] < -1.5f ||
							paper[paperID].snippet[py][px].pos[1] > 1.5f) continue;

						float cornerTexPos[4][2] = {
							{paper[paperID].texPos[py][px][0], paper[paperID].texPos[py][px][1]},
							{paper[paperID].texPos[py][px+1][0], paper[paperID].texPos[py][px+1][1]},
							{paper[paperID].texPos[py+1][px+1][0], paper[paperID].texPos[py+1][px+1][1]},
							{paper[paperID].texPos[py+1][px][0], paper[paperID].texPos[py+1][px][1]}
						};

						float relCornerPos[4][3] =
						{
							{paper[paperID].tilePos[py][px][0] + cornerPos[0][0], paper[paperID].tilePos[py][px][1] + cornerPos[0][1], 0.0f},
							{paper[paperID].tilePos[py][px+1][0] + cornerPos[1][0], paper[paperID].tilePos[py][px+1][1] + cornerPos[1][1], 0.0f},
							{paper[paperID].tilePos[py+1][px+1][0] + cornerPos[2][0], paper[paperID].tilePos[py+1][px+1][1] + cornerPos[2][1], 0.0f},
							{paper[paperID].tilePos[py+1][px][0] + cornerPos[3][0], paper[paperID].tilePos[py+1][px][1] + cornerPos[3][1], 0.0f}
						};

						float nvec[3] = {0.0f, 0.0f, 1.0f};
						rotate(nvec, paper[paperID].snippet[py][px].rpy);
						rotate(relCornerPos[0], paper[paperID].snippet[py][px].rpy);
						rotate(relCornerPos[1], paper[paperID].snippet[py][px].rpy);
						rotate(relCornerPos[2], paper[paperID].snippet[py][px].rpy);
						rotate(relCornerPos[3], paper[paperID].snippet[py][px].rpy);

						float cornerTilePos[4][3] =
						{
							{paper[paperID].snippet[py][px].pos[0] + relCornerPos[0][0],
								   paper[paperID].snippet[py][px].pos[1] + relCornerPos[0][1], 0.99f},
							{paper[paperID].snippet[py][px].pos[0] + relCornerPos[1][0],
								   paper[paperID].snippet[py][px].pos[1] + relCornerPos[1][1], 0.99f},
							{paper[paperID].snippet[py][px].pos[0] + relCornerPos[2][0],
								   paper[paperID].snippet[py][px].pos[1] + relCornerPos[2][1], 0.99f},
							{paper[paperID].snippet[py][px].pos[0] + relCornerPos[3][0],
								   paper[paperID].snippet[py][px].pos[1] + relCornerPos[3][1], 0.99f}
						};

						// Texture coordinates of the points on the stuff.
						float pointTexPos[6][6][2];
						float pointPos[6][6][3];
						for (int yp = 0; yp < 6; yp++)
						{
							for (int xp = 0; xp < 6; xp++)
							{
								float deltaTile = 1.0f / 5.0f;
								float leftAmount = xp * deltaTile;
								float rightAmount = 1.0f - xp * deltaTile;
								float topAmount = yp * deltaTile;
								float bottomAmount = 1.0f - yp * deltaTile;
								float topLeftAmount = leftAmount * topAmount;
								float topRightAmount = rightAmount * topAmount;
								float bottomLeftAmount = leftAmount * bottomAmount;
								float bottomRightAmount = rightAmount * bottomAmount;
								for (int dim = 0; dim < 2; dim++)
								{
									pointTexPos[yp][xp][dim] = topLeftAmount * cornerTexPos[0][dim] +
										topRightAmount * cornerTexPos[1][dim] +
										bottomRightAmount * cornerTexPos[2][dim] +
										bottomLeftAmount * cornerTexPos[3][dim];
								}
								for (int dim = 0; dim < 3; dim++)
								{
									pointPos[yp][xp][dim] = topLeftAmount * cornerTilePos[0][dim] +
										topRightAmount * cornerTilePos[1][dim] +
										bottomRightAmount * cornerTilePos[2][dim] +
										bottomLeftAmount * cornerTilePos[3][dim];
								}

								// displace points based on their position. THIS IS SUPER-HACKY!!!
								//float displaceAmount = paper[paperID].snippet[py][px].detachedTime;
								//if (displaceAmount > 1.0f) displaceAmount = 1.0f;
								//pointPos[yp][xp][0] += displaceAmount * 0.75f * 2.0f / PAPER_Y_TILING * (float)cos(pointPos[yp][xp][1] / 2.0f * PAPER_Y_TILING * 0.75f);
								//pointPos[yp][xp][1] += displaceAmount * 0.75f * 2.0f / PAPER_Y_TILING * (float)cos(pointPos[yp][xp][0] / 2.0f * PAPER_Y_TILING * 0.75f);
								pointPos[yp][xp][0] += 2.0f / PAPER_Y_TILING * nvec[0] * cos(pointPos[yp][xp][0] / 2.0f * PAPER_Y_TILING * 0.75f + pointPos[yp][xp][1] / 2.0f * PAPER_Y_TILING * 0.75f);
								pointPos[yp][xp][1] += 2.0f / PAPER_Y_TILING * nvec[1] * cos(pointPos[yp][xp][0] / 2.0f * PAPER_Y_TILING * 0.75f + pointPos[yp][xp][1] / 2.0f * PAPER_Y_TILING * 0.75f);
							}
						}

						for (int xtile = 0; xtile < 5; xtile++)
						{
							for (int ytile = 0; ytile < 5; ytile++)
							{
								glTexCoord2f(pointTexPos[ytile][xtile][0], pointTexPos[ytile][xtile][1]);
								glVertex3f(pointPos[ytile][xtile][0], pointPos[ytile][xtile][1], pointPos[ytile][xtile][2]);
								glTexCoord2f(pointTexPos[ytile][xtile+1][0], pointTexPos[ytile][xtile+1][1]);
								glVertex3f(pointPos[ytile][xtile+1][0], pointPos[ytile][xtile+1][1], pointPos[ytile][xtile+1][2]);
								glTexCoord2f(pointTexPos[ytile+1][xtile+1][0], pointTexPos[ytile+1][xtile+1][1]);
								glVertex3f(pointPos[ytile+1][xtile+1][0], pointPos[ytile+1][xtile+1][1], pointPos[ytile+1][xtile+1][2]);
								glTexCoord2f(pointTexPos[ytile+1][xtile][0], pointTexPos[ytile+1][xtile][1]);
								glVertex3f(pointPos[ytile+1][xtile][0], pointPos[ytile+1][xtile][1], pointPos[ytile+1][xtile][2]);
							}
						}
					}
				}
			}
			glEnd();
		}
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