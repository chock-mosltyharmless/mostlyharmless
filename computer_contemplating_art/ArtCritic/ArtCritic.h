#pragma once

#include "FeatureCreator.h"

class ArtCritic {
public:
    ArtCritic(void);
    virtual ~ArtCritic(void);

    int CreatePositiveTrainFile(const char *filename);

private:
    FeatureCreator *feature_creator_;
    const static char *train_painters_[];
    const static char *test_painters_[];
};