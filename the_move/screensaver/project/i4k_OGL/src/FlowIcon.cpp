#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "stdlib.h"

#include "FlowIcon.h"
#include "config.h"
#include "TextureManager.h"

const float FlowIcon::borderWidth = 0.02f;
const float FlowIcon::distance = 0.2f;

extern TextureManager textureManager;
extern HWND hWnd;

FlowIcon::FlowIcon(void)
{
	mouseX = 1000.0f;
	mouseY = 1000.0f;
	curTime = -1000.0f;
	clickTime = -2000.0f;
}

FlowIcon::~FlowIcon(void)
{
}

void FlowIcon::init(const char *texName, int xpos, int ypos)
{
	posX = xpos;
	posY = ypos;
	this->texName = texName;
}

float FlowIcon::getGLX(int xpos)
{
	// get to floaty range
	float fpx = xpos * distance;
	// go to -1..1
	fpx = fpx - 1.0f;
	
	return fpx;
}

float FlowIcon::getGLY(int ypos)
{
	// get to floaty range
	float fpy = ypos * distance;
	// apply aspect ratio
	fpy *= ASPECT_RATIO;
	// go to -1..1
	fpy = fpy - 1.0f;
	// go to top-down
	fpy = -fpy;

	return fpy;
}

void FlowIcon::draw(float time)
{
	curTime = time;
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH];
	
	// set texture
	if (textureManager.getTextureID(texName, &texID, errorString))
	{
		MessageBox(hWnd, errorString, "vMainObject shader error", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// Core drawing?
	float xp = getGLX(posX);
	float yp = getGLY(posY);
	float relClickTime = (time - clickTime) * 1.5f;
	if (relClickTime > 1.0f) relClickTime = 1.0f;
	if (relClickTime < 0.0f) relClickTime = 0.0f;
	relClickTime = sqrtf(relClickTime);
	float bw = borderWidth + 0.02f * (1.0f - relClickTime);
	textureManager.drawQuad(xp + bw, yp - (distance - bw) * ASPECT_RATIO,
		                    xp + distance - bw, yp - bw*ASPECT_RATIO,
							relClickTime* 0.2f + 0.8f);
}

void FlowIcon::setMousePosition(float xpos, float ypos)
{
	mouseX = xpos;
	mouseY = ypos;
}

void FlowIcon::clickMouse()
{
	// Check if in range
	float left = posX * distance * 0.5f;
	float right = (posX + 1) * distance * 0.5f;
	float top = posY * (distance*0.5f) * ASPECT_RATIO;
	float bottom = (posY + 1) * (distance*0.5f) * ASPECT_RATIO;

	if (mouseX > left && mouseX < right &&
		mouseY > top && mouseY < bottom)
	{
		clickTime = curTime;
	}
}
