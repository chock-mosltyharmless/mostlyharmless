#pragma once

#define MAX_NUM_BIOMETRIC_DATA 100
#define CONFIG_FILE_NAME "cfp.files"

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
	// Initialize...
	void Init(int width, int height);

	// Writes the thing to file
	void Save(const char *filename) const;

	void FillColor(unsigned char red, unsigned char green,
				   unsigned char blue, unsigned char alpha);

    // Coordinates are in range [0..1] from top left
    // Radius is in width-space
    void FillCircle(float x, float y, float radius,
                    int red, int green, int blue, int alpha);

	// Get a pointer to the data
	typedef unsigned char(*PointerToImage)[3];
	PointerToImage data(void) { return data_; }

private:
	int width_;
	int height_;
	PointerToImage data_;
};
