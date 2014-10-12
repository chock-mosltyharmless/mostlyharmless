#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "Texture.h"
#include "PictureWriter.h" // I need this for the tga definitions. This should be moved somewhere else
#include "intro.h" // I need this for the openGL functions. This should be moved somewhere else

Texture::Texture(void) : textureGLID(0)
{
}

Texture::~Texture(void)
{
}

void Texture::deInit()
{
	glDeleteTextures(1, &textureGLID);
}

HRESULT Texture::init(const char *tgaFileName)
{
	TGAHeader tgaHeader;

	FILE *cfpid = fopen(tgaFileName, "rb");
	if (cfpid == 0) return S_FALSE;

	// load header
	fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);
		
	// load image data
	int textureSize = tgaHeader.width * tgaHeader.height * 4;
	unsigned char *textureData = new unsigned char[textureSize];
	fread(textureData, 1, textureSize, cfpid);
		
	// create openGL texture
	glGenTextures(1, &textureGLID);
	imageWidth = tgaHeader.width;
	imageHeight = tgaHeader.height;
	textureWidth = nextPowerOfTwo(tgaHeader.width);
	textureHeight = nextPowerOfTwo(tgaHeader.height);
	glBindTexture(GL_TEXTURE_2D, textureGLID);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 textureWidth, textureHeight,
				 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
				    tgaHeader.width, tgaHeader.height,
					GL_BGRA, GL_UNSIGNED_BYTE, textureData);
		
	delete [] textureData;
	fclose(cfpid);

	return S_OK;
}

void Texture::setTexture()
{
	//glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureGLID);
}

void Texture::drawScreenAlignedQuad(float startX, float startY, float endX, float endY)
{
	int imgWidth = imageWidth;
	int imgHeight = imageHeight;
	float textureURange = (float)imgWidth / (float)textureWidth;
	float textureVRange = (float)imgHeight / (float)textureHeight;

	float sX = startX;
	float sY = 0.5f * startY - 0.57f; // but why???
	float eX = endX;
	float eY = 0.5f * endY - 0.57f; // but why???
	if (startX < 0.0f)
	{
		sX += 1.0f;
		eX += 1.0f;
	}
	sX = 1.0f - sX;
	eX = 1.0f - eX;
	sX = sX * 640.0f / 1024.0f;
	eX = eX * 640.0f / 1024.0f;
	sY = sY * 480.0f / 512.0f;
	eY = eY * 480.0f / 512.0f;

	setTexture();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, textureVRange);
	glMultiTexCoord2f(GL_TEXTURE1, sX, eY);
	glVertex3f(startX, endY, 0.5);
	glTexCoord2f(textureURange, textureVRange);
	glMultiTexCoord2f(GL_TEXTURE1, eX, eY);
	glVertex3f(endX, endY, 0.5);
	glTexCoord2f(textureURange, 0.0f);
	glMultiTexCoord2f(GL_TEXTURE1, eX, sY);
	glVertex3f(endX, startY, 0.5);
	glTexCoord2f(0.0, 0.0f);
	glMultiTexCoord2f(GL_TEXTURE1, sX, sY);
	glVertex3f(startX, startY, 0.5);
	glEnd();
}

int Texture::nextPowerOfTwo(int value)
{
	int result = 1;
	while (result < value) result *= 2;
	return result;
}
