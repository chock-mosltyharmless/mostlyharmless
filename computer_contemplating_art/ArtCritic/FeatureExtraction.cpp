#include "FeatureExtraction.h"
#include "PictureWriter.h"
#include "stdafx.h"

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

void FeatureExtraction::CreateHistogram(float * data, int num_components, int num_elements,
        float min_value, float max_value, int num_bins, float * histogram) {
    for (int bin = 0; bin < num_bins; bin++) {
        histogram[bin] = 0.0f;
    }
    
    for (int index = 0; index < num_elements; index++) {
        float *element = data + index * num_components;
        for (int dim = 0; dim < num_components; dim++) {
            float value = element[dim];
            int histogram_index = int((value - min_value) / (max_value - min_value) *
                num_bins);
            if (histogram_index < 0) histogram_index = 0;
            if (histogram_index >= num_bins) histogram_index = num_bins - 1;
            histogram[histogram_index * num_components + dim] += 1.0f / num_elements;
        }
    }
}

void FeatureExtraction::calculateFeaturesInternal(void) {
    // Clear all features
    for (int i = 0; i < kFeatureDimension; i++) {
        feature_vector_[i] = 0.0f;
    }

    // Convert to YCbCr
    // TODO: 2 is really blue?
    for (int y = 0; y < kImageHeight; y++) {
        for (int x = 0; x < kImageWidth; x++) {
            int index = y * kImageWidth + x;
            image_ycbcr_[index][0] = 0.299f * image_[y][x][0] +
                0.587f * image_[y][x][1] +
                0.114f * image_[y][x][2];  // Y'
            image_ycbcr_[index][1] = -0.168736f * image_[y][x][0] -
                0.331264f * image_[y][x][1] +
                0.5f * image_[y][x][2];  // Cb
            image_ycbcr_[index][2] = 0.5f * image_[y][x][0] -
                0.418688f * image_[y][x][1] -
                0.0081312f * image_[y][x][2];
            // Scale to 0..1 range
            image_ycbcr_[index][0] *= 256.0f / 219.0f;
            image_ycbcr_[index][1] *= 256.0f / 219.0f;
            image_ycbcr_[index][2] *= 256.0f / 219.0f;
            image_ycbcr_[index][1] += 0.5f;
            image_ycbcr_[index][2] += 0.5f;
        }
    }

    // Create delta images
    for (int i = 0; i < kImageWidth * kImageHeight * 3; i++) {
        image_delta_x_[0][i] = 0.0f;
        image_delta_y_[0][i] = 0.0f;
    }
    for (int y = 1; y < kImageHeight - 1; y++) {
        for (int x = 1; x < kImageWidth - 1; x++) {
            for (int c = 0; c < 3; c++) {
                int index = y * kImageWidth + x;
                int left = index - 1;
                int right = index + 1;
                int top = index - kImageWidth;
                int bottom = index + kImageWidth;
                int top_left = index - kImageWidth - 1;
                int top_right = index - kImageWidth + 1;
                int bottom_left = index + kImageWidth - 1;
                int bottom_right = index + kImageWidth + 1;
                image_delta_x_[index][c] =
                    -image_ycbcr_[top_left][c] +
                    -2.0f * image_ycbcr_[left][c] +
                    -image_ycbcr_[bottom_left][c] +
                    image_ycbcr_[top_right][c] +
                    2.0f * image_ycbcr_[right][c] +
                    image_ycbcr_[bottom_right][c];
                image_delta_x_[index][c] /= 8.0f;  // normalize
                image_delta_x_[index][c] += 0.5f;
                image_delta_y_[index][c] =
                    -image_ycbcr_[top_left][c] +
                    -2.0f * image_ycbcr_[top][c] +
                    -image_ycbcr_[top_right][c] +
                    image_ycbcr_[bottom_left][c] +
                    2.0f * image_ycbcr_[bottom][c] +
                    image_ycbcr_[bottom_right][c];
                image_delta_y_[index][c] /= 8.0f;  // normalize
                image_delta_y_[index][c] += 0.5f;
            }
        }
    }

    // For debug reasons create an image
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_[0], "fe_0.tga");
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_ycbcr_, "fe_1.tga");
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_delta_x_, "fe_2.tga");
    PictureWriter::SaveTGA(kImageWidth, kImageHeight, image_delta_y_, "fe_3.tga");

    // Create histograms
    int feature_index = 0;
    CreateHistogram(image_ycbcr_[0], 3, kImageWidth * kImageHeight,
        0.0f, 1.0f, kHistogramSize, feature_vector_ + feature_index);
    feature_index += 3 * kHistogramSize;
    CreateHistogram(image_delta_x_[0], 3, kImageWidth * kImageHeight, 0.0f, 1.0f,
        kHistogramSize, feature_vector_ + feature_index);
    feature_index += 3 * kHistogramSize;
    CreateHistogram(image_delta_y_[0], 3, kImageWidth * kImageHeight, 0.0f, 1.0f,
        kHistogramSize, feature_vector_ + feature_index);
    feature_index += 3 * kHistogramSize;

    if (feature_index != kFeatureDimension) {
        fprintf(stderr, "Feature dimension %d incorrect. %d is needed.",
            kFeatureDimension, feature_index);
        exit(1);
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
        }
    }
}
