#include "StdAfx.h"
#include "FeatureCreator.h"
#include "stb_image.h"

FeatureCreator::FeatureCreator() {
    feature_extraction_ = new FeatureExtraction();
}


FeatureCreator::~FeatureCreator() {
    delete feature_extraction_;
}

int FeatureCreator::WriteFeatureFile(const char *jpg_directory, FILE *feature_file,
        int label) {
    // Get all files in the directory with .jpg ending
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;

    fprintf(stderr, "\nAdd images from '%s' to feature file\n\n",
        jpg_directory);

    // Go to first file in textures directory
    int dir_size = strlen(jpg_directory);
    int search_size = dir_size + 10;
    char *search_name = new char [search_size + 1];
    sprintf_s(search_name, search_size, "%s/*.jpg", jpg_directory);
    hFind = FindFirstFile(search_name, &ffd);
    delete [] search_name;
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error - could not find files '%s'\n", search_name);
        return -1;
    }

    // Go over all files
    int num_images = 0;
    int num_successful = 0;
    do {
        // Note that the number of textures is increased automatically
        fprintf(stderr, "Loading file %s...\n", ffd.cFileName);
        num_images++;
        int filename_size = strlen(ffd.cFileName);
        int path_size = dir_size + filename_size + 10;
        char *path_name = new char [path_size + 1];
        sprintf_s(path_name, path_size, "%s/%s", jpg_directory, ffd.cFileName);
        int ret_val = JPGFeatures(path_name, feature_file, label);
        if (ret_val >= 0) num_successful++;
        delete [] path_name;
    } while (FindNextFile(hFind, &ffd));

    fprintf(stderr, "\nNumber of images processed in %s: %d (of %d)\n",
            jpg_directory, num_successful, num_images);

    return num_images;  // No error
}

int FeatureCreator::JPGFeatures(const char *filename, FILE *feature_file, int label) {
    const int kNumColorComponents = 3;
    int width;
    int height;
    int num_color_components;
    float *image = stbi_loadf(filename, &width, &height,
        &num_color_components, kNumColorComponents);
    if (NULL == image) {
        fprintf(stderr, "Could not load jpg '%s'\n", filename);
        return -1;
    }
    if (kNumColorComponents != num_color_components) {
        fprintf(stderr, "Image has wrong number of components: %d\n",
                num_color_components);
        stbi_image_free(image);
        return -1;
    }
    if (width < FeatureExtraction::GetPreferredWidth()) {
        fprintf(stderr, "Image width '%s' too small: %d [ < %d ]\n",
            filename, width, FeatureExtraction::GetPreferredWidth());
        stbi_image_free(image);
        return -1;
    }
    if (height < FeatureExtraction::GetPreferredHeight()) {
        fprintf(stderr, "Image height '%s' too small: %d [ < %d ]\n",
            filename, width, FeatureExtraction::GetPreferredHeight());
        stbi_image_free(image);
        return -1;
    }

    float *feature_vector;
    int num_features;
    int ret_val = feature_extraction_->CalculateFeaturesFromFloat(
        &feature_vector, &num_features, (float (*)[3])image, width, height);

    fprintf(feature_file, "%d", label);
    for (int i = 0; i < num_features; i++) {
        float min_value = 0.000005f;
        if (feature_vector[i] < -min_value || feature_vector[i] > min_value) {
            fprintf(feature_file, " %d:%.5f", i + 1, feature_vector[i]);
        }
    }
    fprintf(feature_file, "\n");

    stbi_image_free(image);
    return 0;  // OK
}