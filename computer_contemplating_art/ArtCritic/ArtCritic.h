#pragma once

#include "FeatureCreator.h"

class ArtCritic {
public:
    ArtCritic(void);
    virtual ~ArtCritic(void);

    int CreatePositiveTrainFile(const char *filename);

private:
    FeatureCreator *feature_creator_;
};