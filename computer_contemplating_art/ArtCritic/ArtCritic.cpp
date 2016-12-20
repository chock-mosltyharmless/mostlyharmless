// ArtCritic.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ArtCritic.h"

ArtCritic::ArtCritic(void) {
    feature_creator_ = new FeatureCreator();
}

ArtCritic::~ArtCritic(void) {
    delete feature_creator_;
}

int ArtCritic::CreatePositiveTrainFile(const char *filename) {
    FILE *fid = NULL;
    errno_t error = fopen_s(&fid, filename, "w");
    if (0 != error) {
        fprintf(stderr, "Could not create train file %s\n", filename);
        return -1;
    }

    feature_creator_->WriteFeatureFile("d:/paintings/da_vinci", fid);

    fclose(fid);
    return 0;
}

int main(int argc, char *argv[]) {
    ArtCritic critic;

    critic.CreatePositiveTrainFile("train.txt");

    return 0;
}
