#pragma once

#include "TextureManager.h"
#include "ScreenBorders.h"

#define NUM_PAPER_TEXTURES 9
#define PAPER_PERIOD 8.0
#define PAPER_MOVEMENT_TIME 0.2
// Delay for a paper left to a right one to draw
#define PAPER_MOVEMENT_DELTA 0.25

class MovingPapers
{
public:
	MovingPapers();
	virtual ~MovingPapers(void);

	void draw(float time, HWND mainWnd, TextureManager *texManag, ScreenBorders *borders, bool drawVideo);

private:
	void drawQuad(float left, float bottom, float right, float top, float leftU, float rightU);
	const static char* texNames[NUM_PAPER_TEXTURES];
};

