#include "StdAfx.h"
#include "TextureManager.h"
#include "Configuration.h"
#include "glext.h"


TextureManager::TextureManager(void)
{
	numTextures = 0;
}


TextureManager::~TextureManager(void)
{
}

void TextureManager::releaseAll(void)
{
	glDeleteTextures(numTextures, textureID);
	numTextures = 0;
}

int TextureManager::loadTGA(const char *filename, char *errorString)
{
	char combinedName[TM_MAX_FILENAME_LENGTH+1];

	TGAHeader tgaHeader;
	int numRead;

	sprintf_s(combinedName, TM_MAX_FILENAME_LENGTH,
			  TM_DIRECTORY "%s", filename);

	if (numTextures >= TM_MAX_NUM_TEXTURES)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
		return -1;
	}

	FILE *fid;
	if (fopen_s(&fid, combinedName, "rb"))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Could not load texture '%s'",
				  filename);
		return -1;
	}

	// load header
	numRead = fread(&tgaHeader, 1, sizeof(tgaHeader), fid);
	if (numRead != sizeof(tgaHeader))
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "TGA Header of %s too short",
			      filename);
		return -1;
	}
		
	// load image data
	int textureSize = tgaHeader.width * tgaHeader.height * 4;
	unsigned char *textureData = new unsigned char[textureSize];
	numRead = fread(textureData, 1, textureSize, fid);
	if (numRead != textureSize)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH, "Bogus tga file: '%s'",
			      filename);
		delete [] textureData;
		return -1;
	}
		
	// create openGL texture
	glGenTextures(1, &textureID[numTextures]);
	textureWidth[numTextures] = tgaHeader.width;
	textureHeight[numTextures] = tgaHeader.height;
	glBindTexture(GL_TEXTURE_2D, textureID[numTextures]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				 textureWidth[numTextures], textureHeight[numTextures],
				 0, GL_BGRA, GL_UNSIGNED_BYTE, textureData);
		
	delete [] textureData;
	fclose(fid);
	strcpy_s(textureName[numTextures], TM_MAX_FILENAME_LENGTH, filename);
	numTextures++;

	return 0;
}

int TextureManager::init(char *errorString)
{
	// Free everything if there was something before.
	releaseAll();

	// Go throught the shaders directory and load all shaders.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in textures directory
	char *dirname = TM_DIRECTORY TM_SHADER_WILDCARD;
	hFind = FindFirstFile(TM_DIRECTORY TM_SHADER_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no textures in " TM_DIRECTORY);
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

int TextureManager::getTextureID(const char *name, GLuint *id, char *errorString)
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
	return 0;
}

