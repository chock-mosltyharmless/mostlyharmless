#include "FeatureExtraction.h"



FeatureExtraction::FeatureExtraction() {
}

FeatureExtraction::~FeatureExtraction() {
}

int FeatureExtraction::CalculateFeaturesFromChar(float ** feature_vector, int * feature_dimension,
                                            unsigned char image[][3], int width, int height) {
    // clear features
    *feature_vector = feature_vector_;
    *feature_dimension = kFeatureDimension;

    // Get image to internal format
    ResizeFromChar(image, width, height);

    calculateFeaturesInternal();

    return 0;
}

int FeatureExtraction::CalculateFeaturesFromFloat(float ** feature_vector, int * feature_dimension,
    float image[][3], int width, int height) {
    // clear features
    *feature_vector = feature_vector_;
    *feature_dimension = kFeatureDimension;

    // Get image to internal format
    ResizeFromFloat(image, width, height);

    calculateFeaturesInternal();

    return 0;
}

void FeatureExtraction::calculateFeaturesInternal(void) {
    // Clear all features
    for (int i = 0; i < kFeatureDimension; i++) {
        feature_vector_[i] = 0.0f;
    }

    // Calculate bw histogram
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            int index = (int)(image_bw_[y][x] * kHistogramSize + 0.5f);
            if (index < 0) index = 0;
            if (index >= kHistogramSize) index = kHistogramSize - 1;
            feature_vector_[index] += 1.0f / (kImageHeight * kImageWidth);
        }
    }
}

void FeatureExtraction::ResizeFromChar(unsigned char image[][3], int width, int height) {
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

void FeatureExtraction::ResizeFromFloat(float image[][3], int width, int height) {
    // Create simple point-sampled resize
    // TODO: I need to at least interpolate for down-sampling.
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            // simple down-round of coordinate
            int source_x = x * width / kImageWidth;
            int source_y = y * height / kImageHeight;
            int index = source_y * width + source_x;
            image_[y][x][0] = image[index][0];
            image_[y][x][1] = image[index][1];
            image_[y][x][2] = image[index][2];
            // TODO: is 0 really blue?
            image_bw_[y][x] = 0.21f * image_[y][x][2] +
                0.71f * image_[y][x][1] +
                0.08f * image_[y][x][0];
        }
    }
}

