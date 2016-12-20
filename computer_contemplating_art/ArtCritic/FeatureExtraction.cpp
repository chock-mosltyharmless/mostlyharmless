#include "FeatureExtraction.h"



FeatureExtraction::FeatureExtraction() {
}

FeatureExtraction::~FeatureExtraction() {
}

int FeatureExtraction::CalculateFeatures(float ** feature_vector, int * feature_dimension,
                                         unsigned char image[][3], int width, int height) {
    // clear features
    *feature_vector = feature_vector_;
    *feature_dimension = kFeatureDimension;
    for (int i = 0; i < kFeatureDimension; i++) {
        feature_vector_[0] = 0.0f;
    }

    // Get image to internal format
    Resize(image, width, height);

    return 0;
}

void FeatureExtraction::Resize(unsigned char image[][3], int width, int height) {
    // Create simple point-sampled resize
    // TODO: I need to at least interpolate for down-sampling.
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            // simple down-round of coordinate
            int source_x = x * width / kImageWidth;
            int source_y = y * height / kImageHeight;
            int index = source_y * width + source_x;
            image_[y][x][0] = ((float)(image[index][0])) / 255.0f;
            image_[y][x][1] = ((float)(image[index][1])) / 255.0f;
            image_[y][x][2] = ((float)(image[index][2])) / 255.0f;
            // TODO: is 0 really blue?
            image_bw_[y][x] = 0.21f * image_[y][x][2] +
                0.71f * image_[y][x][1] +
                0.08f * image_[y][x][0];
        }
    }
}
