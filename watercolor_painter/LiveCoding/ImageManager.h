/**
 * The image manager holds all the software textures that the system supports.
 * It has everything in the images/ directory loaded
 */

#pragma once

#define IM_DIRECTORY "images/"
#define IM_SHADER_WILDCARD "*.png"
#define IM_MAX_NUM_IMAGES 64
#define IM_MAX_FILENAME_LENGTH 1024

class ImageManager {
public:
	ImageManager(void);
	~ImageManager(void);

public: // functions
	// Load all textures from the ./images directory.
	// Returns 0 if successful, -1 otherwise
	// The error string must hold space for at least SM_MAX_ERROR_LENGTH
	// characters. It contains errors from compilation/loading
	int Init(char *errorString);

    // Gets a pointer to an 32-bit image
    // Returns 0 if it didn't work
    // Width and height are returned optionally
    unsigned char *GetTexture(const char *name, int *width, int *height, char *error_string);

private: // functions
	void ReleaseAll(void);
    int LoadPNG(const char *filename, char *errorString);
	
private: // data
	int num_images_;
	char image_name_[IM_MAX_NUM_IMAGES][IM_MAX_FILENAME_LENGTH+1];
    unsigned char *image_[IM_MAX_NUM_IMAGES];
	int image_width_[IM_MAX_NUM_IMAGES];
	int image_height_[IM_MAX_NUM_IMAGES];
};

