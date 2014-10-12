#pragma once

#define MAX_NUM_BIOMETRIC_DATA 100
#define CONFIG_FILE_NAME "cfp.files"

// Structure file for the header of a .cfp
struct CFPHeader
{
	int width, height; // of the underlying texture that follows in BGR format
	int numBiometricData; // number of pairs
	float biometricData[MAX_NUM_BIOMETRIC_DATA][2];
};

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

// Saves a picture in .cfp
class PictureWriter
{
public:
	PictureWriter(void);
	~PictureWriter(void);

public:
	// saves to a file, generating a good filename...
	// This also saves a .tga in order to be able to see a preview.
	static void saveFile(CFPHeader *header, BYTE *textureData);

private:
	// load cfp.files and return an unsued ID.
	static int nextFileID();
	// append a file ID to cfp.files
	static void appendFileID(int ID);
};
