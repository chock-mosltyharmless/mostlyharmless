/**
 * The texture manager holds all the textures that the system supports.
 * Additionally to the textures in the ./textures directory, it may also
 * hold some special textures. So far there are none.
 */

#pragma once

#define GLT_DIRECTORY "textures/"
#define GLT_FILE_WILDCARD "*.png"
#define GLT_MAX_NUM_TEXTURES 256
#define GLT_MAX_FILENAME_LENGTH 1024

class GLTextures
{
public:
	GLTextures(void);
	virtual ~GLTextures(void);

	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least MAX_ERROR_LENGTH
	// characters. It contains errors from loading
	int init(char *errorString);

	// Get the texture ID from a named texture
	// That might either be a .png or any of the special textures
	int getTextureID(const char *name, unsigned int *id, char *errorString);

private: // functions
	void releaseAll(void);
	int loadTGA(const char *filename, char *errorString);

private: // data
	int numTextures;
	char textureName[GLT_MAX_NUM_TEXTURES][GLT_MAX_FILENAME_LENGTH+1];
	unsigned int textureID[GLT_MAX_NUM_TEXTURES];
	int textureWidth[GLT_MAX_NUM_TEXTURES];
	int textureHeight[GLT_MAX_NUM_TEXTURES];

};

