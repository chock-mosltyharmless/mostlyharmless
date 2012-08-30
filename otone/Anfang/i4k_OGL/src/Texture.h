#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"

#define NUM_TEXTURES 58
#define TEX_BUTTERFLY_1 0
#define TEX_BUTTERFLY_8 7
#define TEX_BLUEKIMO_1 8
#define TEX_BLUEKIMO_8 15
#define TEX_PAPER_1 16
#define TEX_PAPER_8 23
#define TEX_REDKIMO_1 24
#define TEX_REDKIMO_8 31
#define TEX_SQUARE1 33
#define TEX_NUM_SQUARES 23
#define TEX_BUTTERFLY_BODY 56
#define TEX_BUTTERFLY_WING 57

struct TGAHeader
{
	unsigned char identSize;
	unsigned char colourmapType;
	unsigned char imageType;

	// This is a stupid hack to fool the compiler.
	// I do not know what happens if I compile it
	// under release conditions.
	unsigned char colourmapStart1;
	unsigned char colourmapStart2;
	unsigned char colourmapLength1;
	unsigned char colourmapLength2;
	unsigned char colourmapBits;

	short xStart;
	short yStart;
	short width;
	short height;
	unsigned char bits;
	unsigned char descriptor;
};

class Texture
{
public:
	Texture(void);
	~Texture(void);
	
	int init();
	void deInit();
	void setTexture(int ID) {glBindTexture(GL_TEXTURE_2D, textureGLID[ID]);}
	void drawScreenAlignedQuad(float color[4],
							   float startX = -1.0f, float startY = -1.0f,
							   float endX = 1.0f, float endY = 1.0f);

public:
	static int nextPowerOfTwo(int value);
	//int textureWidth;  // real width in opengl (no padding)
	//int textureHeight; // real height in opengl (no padding)
	int imageWidth;
	int imageHeight;
	GLuint textureGLID[NUM_TEXTURES];
};

