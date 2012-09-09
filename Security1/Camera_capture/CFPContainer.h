#pragma once

// requires PictureWriter.h for definitions
#include "PictureWriter.h"

class CFPContainer
{
public:
	CFPContainer(void);
	~CFPContainer(void);
	// load the cfp.files and the corresponding textures.
	// Call this after openGL initialization
	HRESULT init(const char *filePref);
	void deInit();
	int getNumTextures() {return numTextures;}
	CFPHeader *getHeader(int index) {return headers + index;}
	void setTexture(int index);
	void drawScreenAlignedQuad(int index,
							   float startX = -1.0f, float startY = -1.0f,
							   float endX = 1.0f, float endY = 1.0f);
	void drawBiometric(int index,
					   float startX = -1.0f, float startY = -1.0f,
					   float endX = 1.0f, float endY = 1.0f);
	void drawMorphedBiometric(int index, int seed,
							  float startX = -1.0f, float startY = -1.0f,
							  float endX = 1.0f, float endY = 1.0f);

private:
	static int nextPowerOfTwo(int value);
	static void drawCircle(float xpos1, float ypos1, float xpos2, float ypos2);
	static void drawPolygon(int numEdges, float coordinates[][2],
			                float startX, float startY,
			                float endX, float endY);
	static void drawLongLine(int numEdges, float coordinates[][2],
			                 float startX, float startY,
			                 float endX, float endY);
	static void drawBiometric(float biometricData[][2],
							  float startX = -1.0f, float startY = -1.0f,
							  float endX = 1.0f, float endY = 1.0f);
	static float random();
	void getBoundingBox(int index, float *boundingBox);
	static void getBoundingBox(float biometricData[][2], int numBiometricData, float *boundingBox);

private:
	int numTextures;
	CFPHeader *headers;
	int *textureWidth;  // real width in opengl (with padding)
	int *textureHeight; // real height in opengl (with padding)
	GLuint *textureGLID;
};
