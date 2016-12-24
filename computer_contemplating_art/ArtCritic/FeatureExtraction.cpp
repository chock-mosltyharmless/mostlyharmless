#include "FeatureExtraction.h"
#include "PictureWriter.h"

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
    // Clamp to target aspect ratio
    float target_aspect_ratio = (float)GetPreferredWidth() / (float)GetPreferredHeight();
    float aspect_ratio = (float)width / (float)height;
    float x_offset = 0.0f;
    float adjusted_width = (float)width;
    float y_offset = 0.0f;
    float adjusted_height = (float)height;
    if (aspect_ratio > target_aspect_ratio) {
        adjusted_width = height * target_aspect_ratio;
        x_offset = (width - adjusted_width) / 2.0f;
    } else {
        adjusted_height = width / target_aspect_ratio;
        y_offset = (height - adjusted_height) / 2.0f;
    }

    // Create simple point-sampled resize
    // TODO: I need to at least interpolate for down-sampling.
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            // simple down-round of coordinate
            int source_x = (int)(x * adjusted_width / kImageWidth + x_offset);
            int source_y = (int)(y * adjusted_height / kImageHeight + y_offset);
            if (source_x >= width) source_x = width - 1;
            if (source_x < 0) source_x = 0;
            if (source_y >= height) source_y = height - 1;
            if (source_y < 0) source_y = 0;
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
    // For debug reasons create an image
    PictureWriter::SaveTGA(width, height, image, "fe_0.tga");

    // Clamp to target aspect ratio
    float target_aspect_ratio = (float)GetPreferredWidth() / (float)GetPreferredHeight();
    float aspect_ratio = (float)width / (float)height;
    float x_offset = 0.0f;
    float adjusted_width = (float)width;
    float y_offset = 0.0f;
    float adjusted_height = (float)height;
    if (aspect_ratio > target_aspect_ratio) {
        adjusted_width = height * target_aspect_ratio;
        x_offset = (width - adjusted_width) / 2.0f;
    }
    else {
        adjusted_height = width / target_aspect_ratio;
        y_offset = (height - adjusted_height) / 2.0f;
    }

    // Create simple point-sampled resize
    // TODO: I need to at least interpolate for down-sampling.
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            // simple down-round of coordinate
            int source_x = (int)(x * adjusted_width / kImageWidth + x_offset);
            int source_y = (int)(y * adjusted_height / kImageHeight + y_offset);
            if (source_x >= width) source_x = width - 1;
            if (source_x < 0) source_x = 0;
            if (source_y >= height) source_y = height - 1;
            if (source_y < 0) source_y = 0;
            int index = source_y * width + source_x;
            image_[y][x][0] = image[index][0];
            image_[y][x][1] = image[index][1];
            image_[y][x][2] = image[index][2];
            // TODO: is 0 really blue?
            image_bw_[y][x] = 0.08f * image_[y][x][2] +
                0.71f * image_[y][x][1] +
                0.21f * image_[y][x][0];
        }
    }

    // Create delta images
    for (int i = 0; i < kImageWidth * kImageHeight * 3; i++) {
        image_delta_x[0][0][i] = 0.0f;
        image_delta_y[0][0][i] = 0.0f;
    }
    for (int y = 1; y < kImageHeight - 1; y++) {
        for (int x = 1; x < kImageWidth - 1; x++) {
            for (int c = 0; c < 3; c++) {
                image_delta_x[y][x][c] =
                    -image_[y - 1][x - 1][c] +
                    -2.0f * image_[y][x - 1][c] +
                    -image_[y + 1][x - 1][c] +
                    image_[y - 1][x + 1][c] +
                    2.0f * image_[y][x + 1][c] +
                    image_[y + 1][x + 1][c];
                image_delta_x[y][x][c] /= 8.0f;  // normalize
                image_delta_x[y][x][c] += 0.5f;
                image_delta_y[y][x][c] =
                    -image_[y - 1][x - 1][c] +
                    -2.0f * image_[y - 1][x][c] +
                    -image_[y - 1][x + 1][c] +
                    image_[y + 1][x - 1][c] +
                    2.0f * image_[y + 1][x][c] +
                    image_[y + 1][x + 1][c];
                image_delta_y[y][x][c] /= 8.0f;  // normalize
                image_delta_y[y][x][c] += 0.5f;
            }
        }
    }

    // For debug reasons create an image
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_[0], "fe_1.tga");
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_delta_x[0], "fe_2.tga");
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_delta_y[0], "fe_3.tga");
}

