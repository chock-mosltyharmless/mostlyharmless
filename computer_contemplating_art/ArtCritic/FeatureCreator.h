#pragma once

#include "stdafx.h"
#include "FeatureExtraction.h"

// Creates feature files from jpgs
class FeatureCreator
{
public:
    FeatureCreator();
    virtual ~FeatureCreator();

    // Returns the number of processed files (or -1 on error)
    int WriteFeatureFile(const char *jpg_directory, FILE *feature_file);

private:
    int LoadJPG(const char *filename);
};

