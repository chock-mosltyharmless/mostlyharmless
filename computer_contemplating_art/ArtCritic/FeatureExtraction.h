#pragma once
class FeatureExtraction
{
public:
    FeatureExtraction();
    virtual ~FeatureExtraction();
    
    // Returned feature vector may not be freed and is valid until next call to the function
    // The feature dimension is a return value
    // image must be 8 bit RGB, width == stride [blue == 0]
    // returns negative value on error
    int CalculateFeatures(float **feature_vector, int *feature_dimension,
        unsigned char image[][3], int width, int height);

    static int GetPreferredWidth(void) { return kImageWidth; }
    static int GetPreferredHeight(void) { return kImageHeight; }

private:
    // Saves the result in image_
    // Saves a grayscale version of the image in image_bw_
    void Resize(unsigned char image[][3], int width, int height);

    const static int kFeatureDimension = 6 * 3;
    float feature_vector_[kFeatureDimension];

    const static int kImageWidth = 512;
    const static int kImageHeight = 256;
    // Float due to resize/normalization.
    // Blue must be 0
    float image_[kImageHeight][kImageWidth][3];
    // Grayscale version of the image
    float image_bw_[kImageHeight][kImageWidth];
};

