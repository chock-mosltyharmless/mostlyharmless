#pragma once
class FeatureExtraction
{
public:
    FeatureExtraction();
    virtual ~FeatureExtraction();
    
    // Returned feature vector may not be freed and is valid until next call to the function
    // The feature dimension is a return value
    // image must be 8 bit RGB, width == stride [red == 0, blue == 2]
    // returns negative value on error
    int CalculateFeaturesFromChar(float **feature_vector, int *feature_dimension,
        unsigned char image[][3], int width, int height);
    int CalculateFeaturesFromFloat(float **feature_vector, int *feature_dimension,
        float image[][3], int width, int height);

    static int GetPreferredWidth(void) { return kImageWidth; }
    static int GetPreferredHeight(void) { return kImageHeight; }

private:
    // Saves the result in image_
    // Saves a grayscale version of the image in image_bw_
    void ResizeFromChar(unsigned char image[][3], int width, int height);
    void ResizeFromFloat(float image[][3], int width, int height);
    static void CreateHistogram(float *data, int num_components, int num_elements,
        float min_value, float max_value, int num_bins, float *histogram);

    // Uses images_ and image_bw_
    // Writes to feature_vector_
    void calculateFeaturesInternal(void);

    const static int kImageWidth = 512;
    const static int kImageHeight = 256;
    const static int kHistogramSize = 9;
    const static int kMaxImageScaling = 16;  // Smallest image is kImageHeight / kMaxImageScaling

    const static int kFeatureDimension = 5 * 9 * kHistogramSize;
    float feature_vector_[kFeatureDimension];

    // Float due to resize/normalization.
    // Blue must be 0
    float image_[kImageHeight][kImageWidth][3];
    // YCbCr version of the image
    float image_ycbcr_[kImageHeight * kImageWidth][3];
    float image_delta_x_[kImageHeight * kImageWidth][3];
    float image_delta_y_[kImageHeight * kImageWidth][3];
};

