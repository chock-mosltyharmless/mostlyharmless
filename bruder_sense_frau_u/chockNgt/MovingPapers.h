#pragma once

#include "TextureManager.h"

#define NUM_PAPERS 9
#define NUM_PAPER_TEXTURES NUM_PAPERS
#define PAPER_PERIOD 3.0f
#define PAPER_MOVEMENT_TIME 0.5f
// Delay for a paper left to a right one to draw
#define PAPER_MOVEMENT_DELTA 0.0f

// important for ripping papers apart
#define PAPER_X_TILING 10
#define PAPER_Y_TILING 30

class PaperSnippet
{
public:
	bool attached; // If true, firmly attached to corresponding paper
	float pos[3]; // But we simply ignore Z
	float speed[3]; // Movement speed, should be 0 along normal
	float rpy[3]; // Rotation stored as roll, pitch and yaw

	// Somehow I need to store the displacement
	// Or maybe it is part of the paper?
};

class Paper
{
public:
	// Position of the main part (not counting the ripped-off stuff)
	float pos[2];

	// The size of one paper is always 2/3 to 2/1

	PaperSnippet snippet[PAPER_Y_TILING][PAPER_X_TILING];
	float texPos[PAPER_Y_TILING+1][PAPER_X_TILING+1][2];
	// Relative to pos? Or alternatively to PaperSnippet's pos?
	float tilePos[PAPER_Y_TILING+1][PAPER_X_TILING+1][3];
};

class MovingPapers
{
public:
	MovingPapers();
	virtual ~MovingPapers(void);

	// Initialize positions and stuff (TODO: Add wrapper for different runs)
	void init();
	void update(float deltaTime);
	void draw(HWND mainWnd, TextureManager *texManag, bool drawVideo);

private:
	void drawQuad(float left, float bottom, float right, float top, float leftU, float rightU);
	void rotate(float pos[3], float rpy[3]);
	
	const static char* texNames[NUM_PAPER_TEXTURES];

	Paper paper[NUM_PAPERS];

	// Overall time since init();
	float time;
};

