#pragma once

#include "TextureManager.h"

#define NUM_PAPERS 6
#define NUM_PAPER_TEXTURES 104
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
	float detachedTime; // Time since it is no longer attached
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
	int texIdx;

	// The size of one paper is always 2/3 to 2/1
	
	bool updatePos;

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
	void update(float deltaTime, bool noMovement);
	// Set texName to some valid name to not use papers
	void draw(HWND mainWnd, TextureManager *texManag, bool useStaticTexture,  GLuint texID);

	// Start dropping snippets.
	void startDetaching(void) {doDetach = true; detachingTime = 0.0f;}
	void stopFeeding(void) {doFeeding = false;}

private:
	void drawQuad(float left, float bottom, float right, float top, float leftU, float rightU);
	void rotate(float pos[3], float rpy[3]);
	
	const static char* texNames[NUM_PAPER_TEXTURES];

	Paper paper[NUM_PAPERS];

	// Overall time since init();
	float time;
	float detachingTime; // Time since dismantling started
	bool doDetach; // The papers transform into snippets
	bool doFeeding; // Feed new papers from left when papers move out to the right
};

