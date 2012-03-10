#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glext.h"
#include "Texture.h"

static char TgaFileNames[NUM_TEXTURES][256] =
{
	"textures/butterfly1.tga",
	"textures/butterfly2.tga",
	"textures/butterfly3.tga",
	"textures/butterfly4.tga",
	"textures/butterfly5.tga",
	"textures/butterfly6.tga",
	"textures/butterfly7.tga",
	"textures/butterfly8.tga",
	"textures/bluekimo1.tga",
	"textures/bluekimo2.tga",
	"textures/bluekimo3.tga",
	"textures/bluekimo4.tga",
	"textures/bluekimo5.tga",
	"textures/bluekimo6.tga",
	"textures/bluekimo7.tga",
	"textures/bluekimo8.tga",
	"textures/paper1.tga",
	"textures/paper2.tga",
	"textures/paper3.tga",
	"textures/paper4.tga",
	"textures/paper5.tga",
	"textures/paper6.tga",
	"textures/paper7.tga",
	"textures/paper8.tga",
	"textures/redkimo1.tga",
	"textures/redkimo2.tga",
	"textures/redkimo2.tga",
	"textures/redkimo3.tga",
	"textures/redkimo5.tga",
	"textures/redkimo5.tga",
	"textures/redkimo4.tga",
	"textures/redkimo6.tga",
	"textures/background.tga"
};

Texture::Texture(void)
{
	for (int i = 0; i < NUM_TEXTURES; i++)
	{
		textureGLID[i] = -1;
	}
}

Texture::~Texture(void)
{
}

void Texture::deInit()
{
	glDeleteTextures(NUM_TEXTURES, textureGLID);
}

int Texture::init()
{
	TGAHeader tgaHeader;

	glGenTextures(NUM_TEXTURES, textureGLID);
	for (int i = 0; i < NUM_TEXTURES; i++)
	{
		FILE *cfpid = fopen(TgaFileNames[i], "rb");
		if (cfpid == 0) return 0;

		// load header
		fread(&tgaHeader, 1, sizeof(tgaHeader), cfpid);
		
		// load image data
		int textureSize = tgaHeader.width * tgaHeader.height * 4;
		unsigned char *textureData = new unsigned char[textureSize];
		fread(textureData, 1, textureSize, cfpid);
		
		// create openGL texture
		imageWidth = tgaHeader.width;
		imageHeight = tgaHeader.height;
		glBindTexture(GL_TEXTURE_2D, textureGLID[i]);
		// TODO: Mip Mapping!!!!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
		glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
#if 0
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					 imageWidth, imageHeight,
					 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#else
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, imageWidth, imageHeight,
					      GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#endif

		delete [] textureData;
		fclose(cfpid);
	}

	return 1;
}

void Texture::drawScreenAlignedQuad(float color[3], float startX, float startY,
									float endX, float endY)
{
	//float TEXTURE_U_RANGE = (float)imgWidth / (float)textureWidth;
	//float TEXTURE_V_RANGE = (float)imgHeight / (float)textureHeight;

	glBegin(GL_QUADS);
	glColor3fv(color);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(startX, endY, 0.5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(endX, endY, 0.5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(endX, startY, 0.5);
	glTexCoord2f(0.0, 0.0f);
	glVertex3f(startX, startY, 0.5);
	glEnd();
}

int Texture::nextPowerOfTwo(int value)
{
	int result = 1;
	while (result < value) result *= 2;
	return result;
}
