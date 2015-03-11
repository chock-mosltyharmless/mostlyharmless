#pragma once

#include "TextureManager.h"

// Used to be able to project onto 3 Schraenke.
// I will use Parameters for that later on.
class ScreenBorders
{
public:
	ScreenBorders(void);
	virtual ~ScreenBorders(void);

	void drawBorders(TextureManager *tex, HWND mainWnd, bool showBlue, float opacity, float redenner);

public:
	void drawQuad(float left, float bottom, float right, float top, float tx1, float tx2);
	// segmentation left then right (leftmost Schrank first)
	float xBorder[3][2];
	// segmentation bottom then top (OpenGL orientation)
	float yBorder[3][2];
};

