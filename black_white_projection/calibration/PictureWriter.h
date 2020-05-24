#pragma once

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
    static void SaveTGA(int width, int height, float (*texture_data)[3], const char *filename);
    static void SaveTGA(int width, int height, int (*data)[2], const char *filename, int max_value);
};
