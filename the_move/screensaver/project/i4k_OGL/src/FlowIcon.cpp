#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include <stdlib.h>

#include "FlowIcon.h"
#include "config.h"
#include "TextureManager.h"

extern TextureManager textureManager;
extern HWND hWnd;

extern int realXRes;
extern int realYRes;

FlowIcon::FlowIcon(void)
{
}

FlowIcon::~FlowIcon(void)
{
}

void FlowIcon::init(const char *texName, float xpos, float ypos, float distance, float borderWidth)
{
	posX = xpos;
	posY = ypos;
	this->texName = texName;
	this->distance = distance;
	this->borderWidth = borderWidth;

	mouseX = 1000.0f;
	mouseY = 1000.0f;
	curTime = -1000.0f;
	clickTime = -2000.0f;
	mouseOverAmount = 0.0f;
	lastDrawTime = -2000.0f;
	mouseIsOver = false;
}

float FlowIcon::getGLX()
{
	return posX;
}

float FlowIcon::getGLY()
{
	return posY;
}

void FlowIcon::drawAlarming(float time, float xDelta)
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH];

	float alarmAmount = (sin(time * 2.5f) + 0.8f) / 1.8f;
	if (alarmAmount < 0.0f) alarmAmount = 0.0f;
	alarmAmount = sqrtf(alarmAmount);
	alarmAmount = sqrtf(alarmAmount);

	if ((time * 2.5f) > 5.0f) alarmAmount = 0.0f;

	// set texture
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (textureManager.getTextureID(texName, &texID, errorString))
	{
		MessageBox(hWnd, errorString, "vMainObject shader error", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// Core drawing?
	float xp = getGLX() + xDelta;
	float yp = getGLY();
	yp += alarmAmount * 0.1f + 0.05f * alarmAmount*cos(time*33.0f);
	float bw = borderWidth;
	float bwx = bw - 0.03f * alarmAmount * ((sin(8.3f * time + 15.2f) + sin(7.2f * time + 13.5f)) + 0.4f);
	float bwy = bw - 0.03f * alarmAmount * ((sin(8.3f * time + 15.2f) + sin(7.2f * time + 13.5f)) + 0.4f);
	textureManager.drawQuad(xp + bwx, yp - (distance - bwy) * ASPECT_RATIO,
		                    xp + distance - bwx, yp - bwy*ASPECT_RATIO,
							1.0f, 0.4f*alarmAmount * sin(time*25.0f));
}

void FlowIcon::drawSubCategory(float time)
{
	curTime = time;
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH];

	if (mouseIsOver)
	{
		mouseOverAmount += (time-lastDrawTime)*(1.0f - mouseOverAmount)*8.0f;
		if (mouseOverAmount > 1.0f) mouseOverAmount = 1.0f;
	}
	else
	{
		float decay = exp(-(time-lastDrawTime)*6.0f);
		mouseOverAmount *= decay;
	}

	// Draw the background
	if (textureManager.getTextureID("blue.tga", &texID, errorString))
	{
		MessageBox(hWnd, errorString, "texture not found", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	float xp = getGLX();
	float yp = getGLY();
	float bw = borderWidth * 0.5f;
	textureManager.drawQuad(xp + bw, yp - (distance - bw) * ASPECT_RATIO,
							xp + distance - bw, yp - bw*ASPECT_RATIO,
							mouseOverAmount);

	// set texture
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (textureManager.getTextureID(texName, &texID, errorString))
	{
		MessageBox(hWnd, errorString, "vMainObject shader error", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// Core drawing?
	xp = getGLX();
	yp = getGLY();
	bw = borderWidth;
	textureManager.drawQuad(xp + bw, yp - (distance - bw) * ASPECT_RATIO,
		                    xp + distance - bw, yp - bw*ASPECT_RATIO,
							1.0f);
	lastDrawTime = time;
}


void FlowIcon::drawAmount(float mouseOverAmount, float relClickTime, float time, float xDelta, float yDelta)
{
	GLuint texID;
	char errorString[MAX_ERROR_LENGTH];

	// set texture
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (textureManager.getTextureID(texName, &texID, errorString))
	{
		MessageBox(hWnd, errorString, "vMainObject shader error", MB_OK);
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, texID);

	// Core drawing?
	float xp = getGLX() + xDelta;
	float yp = getGLY() + yDelta;
	float bw = borderWidth + 0.03f * (1.0f - relClickTime);
	float bwx = bw + 0.002f * mouseOverAmount * (sin(8.3f * time + 5.2f) + sin(7.2f * time + 3.5f));
	float bwy = bw + 0.002f * mouseOverAmount * (sin(3.2f * time + 2.2f) + sin(9.1f * time + 1.1f));
	textureManager.drawQuad(xp + bwx, yp - (distance - bwy) * ASPECT_RATIO,
		                    xp + distance - bwx, yp - bwy*ASPECT_RATIO,
							1.0f);
	lastDrawTime = time;
}

void FlowIcon::draw(float time, float xDelta)
{
	curTime = time;
	
	if (mouseIsOver)
	{
		mouseOverAmount += (time-lastDrawTime)*(1.02f - mouseOverAmount)*6.0f;
		if (mouseOverAmount > 1.0f)
		{
			// The mouse was over a long time, ignore any more until it's over again
			mouseIsOver = false;
			mouseOverAmount = 1.0f;
		};
	}
	else
	{
		float decay = exp(-(time-lastDrawTime)*4.0f);
		mouseOverAmount *= decay;
	}

	float relClickTime = (time - clickTime) * 1.5f;
	if (relClickTime > 1.0f) relClickTime = 1.0f;
	if (relClickTime < 0.0f) relClickTime = 0.0f;
	relClickTime = sqrtf(relClickTime);

	drawAmount(mouseOverAmount, relClickTime, time, xDelta);
}

void FlowIcon::setMousePosition(float xpos, float ypos)
{
	// Other direction of y...
	ypos = 1.0f - ypos;

	// Check if in range
	float left = (posX + 1.0f + borderWidth) * 0.5f;
	float right = (posX + 1.0f + distance - borderWidth) * 0.5f;
	float top = (posY + 1.0f - (distance-borderWidth)*ASPECT_RATIO) * 0.5f;
	float bottom = (posY + 1.0f + borderWidth*ASPECT_RATIO) * 0.5f;
	mouseX = xpos;
	mouseY = ypos;

	if (mouseX > left && mouseX < right &&
		mouseY > top && mouseY < bottom)
	{
		mouseIsOver = true;
	}
	else
	{
		mouseIsOver = false;
	}
}

bool FlowIcon::clickMouse()
{
	if (mouseIsOver)
	{
		clickTime = curTime;
		return true;
	}
	return false;
}
