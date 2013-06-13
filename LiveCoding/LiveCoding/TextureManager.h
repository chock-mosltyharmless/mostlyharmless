/**
 * The texture manager holds all the textures that the system supports.
 * Additionally to the textures in the ./textures directory, it also
 * holds some special textures. Here is a list of textures:
 * TODO: Shall the texture manager handle the render target stuff or not????
 * - 'noise2D': Core 2D noise texture. In the future there might be more
 * - 'renderTarget': Core render target at render target resolution
 * - 'smallTarget': Smaller render target for hypnoglow and the like
 */

#pragma once

#define TM_DIRECTORY "textures/"
#define TM_SHADER_WILDCARD "*.tga"
#define TM_MAX_NUM_TEXTURES 64
#define TM_MAX_FILENAME_LENGTH 1024

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

class TextureManager
{
public:
	TextureManager(void);
	~TextureManager(void);

public: // functions
	// Load all textures from the ./textures directory.
	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least SM_MAX_ERROR_LENGTH
	// characters. It contains errors from compilation/loading
	int init(char *errorString);

	// Get the texture ID from a named texture
	// That might either be a .tga or any of the special textures
	int getTextureID(const char *name, GLuint *id, char *errorString);

private: // functions
	void releaseAll(void);
	int loadTGA(const char *filename, char *errorString);

private: // data
	int numTextures;
	char textureName[TM_MAX_NUM_TEXTURES][TM_MAX_FILENAME_LENGTH+1];
	GLuint textureID[TM_MAX_NUM_TEXTURES];
	int textureWidth[TM_MAX_NUM_TEXTURES];
	int textureHeight[TM_MAX_NUM_TEXTURES];
};

