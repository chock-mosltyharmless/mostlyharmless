#pragma once

#include "FeatureCreator.h"

class ArtCritic {
public:
    ArtCritic(void);
    virtual ~ArtCritic(void);

    int CreatePositiveFeatureFile(const char *filename,
        const char **file_list, int file_list_length);

private:
    FeatureCreator *feature_creator_;
};