#include "StdAfx.h"
#include "GLTextures.h"
#include "global.h"
#include "gl/glu.h"
#include "wglext.h"
#define LODEPNG_NO_COMPILE_CPP
#include "lodepng.h"

GLTextures::GLTextures(void)
{
	numTextures = 0;
}

GLTextures::~GLTextures(void)
{
}

void GLTextures::releaseAll(void)
{
	glDeleteTextures(numTextures, textureID);
	numTextures = 0;
}

int GLTextures::loadTGA(const char *filename, char *errorString)
{
	char combinedName[GLT_MAX_FILENAME_LENGTH+1];

	sprintf_s(combinedName, GLT_MAX_FILENAME_LENGTH,
			  GLT_DIRECTORY "%s", filename);

	if (numTextures >= GLT_MAX_NUM_TEXTURES)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

	DWORD *pngData;
	unsigned int width, height;
	unsigned int loadResult =
			lodepng_decode32_file((unsigned char**)&pngData, &width, &height,
								  combinedName);
	if (loadResult != 0)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Loading PNG '%s' failed, "
				  "lodepng error code %d", combinedName, loadResult);
		if (pngData) free(pngData);
		return -1;
	}
		
	// create openGL texture
	glGenTextures(1, &textureID[numTextures]);
	textureWidth[numTextures] = width;
	textureHeight[numTextures] = height;
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
#if 0 // Use simple mipmap generation	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 textureWidth[numTextures], textureHeight[numTextures],
				 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
#else
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
					  textureWidth[numTextures], textureHeight[numTextures],
					  GL_RGBA, GL_UNSIGNED_BYTE, pngData);
#endif

	free(pngData);
	strcpy_s(textureName[numTextures], GLT_MAX_FILENAME_LENGTH, filename);
	numTextures++;

	return 0;
}

int GLTextures::init(char *errorString)
{
	// Free everything if there was something before.
	releaseAll();

	// Go throught the textures directory and load all textures.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in textures directory
	hFind = FindFirstFile(GLT_DIRECTORY GLT_FILE_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no textures in " GLT_DIRECTORY);
		return -1;
	}

	// Load all the textures in the directory
	do
	{		
		// Note that the number of textures is increased automatically
		int retVal = loadTGA(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	return 0;
}

int GLTextures::getTextureID(const char *name, GLuint *id, char *errorString)
{
	for (int i = 0; i < numTextures; i++)
	{
		if (strcmp(name, textureName[i]) == 0)
		{
			*id = textureID[i];
			return 0;
		}
	}

	// Got here without finding a texture, return error.
	sprintf_s(errorString, MAX_ERROR_LENGTH,
			  "Could not find texture '%s'", name);
	return -1;
}