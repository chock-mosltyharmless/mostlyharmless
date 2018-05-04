#include "StdAfx.h"
#include "ImageManager.h"
#include "Configuration.h"
#include "glext.h"
#include "gl/glu.h"
#include "GLNames.h"
#include "Parameter.h"
#include "stb_image.h"

ImageManager::ImageManager(void) {
	num_images_ = 0;
    for (int i = 0; i < IM_MAX_NUM_IMAGES; i++) {
        image_[i] = NULL;
    }
}

ImageManager::~ImageManager(void) {
	ReleaseAll();
}

void ImageManager::ReleaseAll(void) {
	num_images_ = 0;
    for (int i = 0; i < IM_MAX_NUM_IMAGES; i++) {
        if (image_[i]) stbi_image_free(image_[i]);
        image_[i] = NULL;
    }
}

int ImageManager::LoadPNG(const char *filename, char *errorString) {
    char combinedName[IM_MAX_FILENAME_LENGTH + 1];

    sprintf_s(combinedName, IM_MAX_FILENAME_LENGTH,
        IM_DIRECTORY "%s", filename);

    if (num_images_ >= IM_MAX_NUM_IMAGES) {
        sprintf_s(errorString, MAX_ERROR_LENGTH, "Too many textures.");
        return -1;
    }

    int width, height, num_components;
    image_[num_images_] = stbi_load(combinedName, &width, &height, &num_components, 4);
    unsigned char *data = image_[num_images_];
    image_width_[num_images_] = width;
    image_height_[num_images_] = height;

    // Pre-multiply alpha
    for (int pos = 0; pos < width * height; pos++) {
        int index = pos * 4;
        int alpha = 3;
        for (int color = 0; color < 3; color++) {
            data[index + color] = (((int)data[index + color]) *
                ((int)data[index + alpha])) / 255;
        }
    }

    strcpy_s(image_name_[num_images_], IM_MAX_FILENAME_LENGTH, filename);
    num_images_++;

    return 0;
}

int ImageManager::Init(char *errorString) {
	// Free everything if there was something before.
	ReleaseAll();

	// Go throught the textures directory and load all textures.
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;

	// Go to first file in textures directory
	char *dirname = IM_DIRECTORY IM_SHADER_WILDCARD;
	hFind = FindFirstFile(IM_DIRECTORY IM_SHADER_WILDCARD, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		sprintf_s(errorString, MAX_ERROR_LENGTH,
				  "IO Error\nThere are no textures in " IM_DIRECTORY);
		return -1;
	}

	// Load all the textures in the directory
	do
	{		
		// Note that the number of textures is increased automatically
		int retVal = LoadPNG(ffd.cFileName, errorString);
		if (retVal) return retVal;
	} while (FindNextFile(hFind, &ffd));

	return 0;
}

unsigned char *ImageManager::GetTexture(const char *name, int *width, int *height, char *error_string) {
	int image_index;
	bool image_found = false;
    unsigned char *result;
	for (int i = 0; i < num_images_ && !image_found; i++) {
		if (strcmp(name, image_name_[i]) == 0) {
			result = image_[i];
            if (width) *width = image_width_[i];
            if (height) *height = image_height_[i];
			image_found = true;
		}
	}

	if (!image_found) {
		if (error_string) sprintf_s(error_string, MAX_ERROR_LENGTH,
			"Could not find texture '%s'", name);
		return NULL;
	}

	// id was correctly set at this point.
	return result;
}