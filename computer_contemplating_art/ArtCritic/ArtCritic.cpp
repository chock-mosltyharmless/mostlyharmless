// ArtCritic.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ArtCritic.h"

const char *ArtCritic::train_painters_[] = {
    "d:/paintings/artemisia/",
    "d:/paintings/bacon/",
    "d:/paintings/blake/",
    "d:/paintings/bondone/",
    "d:/paintings/bosch/",
    "d:/paintings/botticelli/",
    "d:/paintings/bouninsegna/",
    "d:/paintings/braque/",
    "d:/paintings/bruegel/",
    "d:/paintings/buonarroti/",
    "d:/paintings/caravaggio/",
    "d:/paintings/cezanne/",
    "d:/paintings/chagall/",
    "d:/paintings/chirico/",
    "d:/paintings/cimabue/",
    "d:/paintings/cleanup.sh*",
    "d:/paintings/constable/",
    "d:/paintings/corot/",
    "d:/paintings/courbet/",
    "d:/paintings/da_vinci/",
    "d:/paintings/dali/",
    "d:/paintings/david/",
    "d:/paintings/degas/",
    "d:/paintings/delacroix/",
    "d:/paintings/duchamps/",
    "d:/paintings/duerer/",
    "d:/paintings/ensor/",
    "d:/paintings/ernst/",
    "d:/paintings/eyck/",
    "d:/paintings/francesca/",
    "d:/paintings/friedrich/",
    "d:/paintings/gauguins/",
    "d:/paintings/gericault/",
    "d:/paintings/giorgione/",
    "d:/paintings/goya/",
    "d:/paintings/greco/",
    "d:/paintings/gris/",
    "d:/paintings/hals/",
    "d:/paintings/hockney/",
    "d:/paintings/hogarth/",
    "d:/paintings/holbein/",
    "d:/paintings/homer/",
    "d:/paintings/hopper/",
    "d:/paintings/ingres/",
    "d:/paintings/japanese/",
    "d:/paintings/johns/",
    "d:/paintings/kahlo/",
    "d:/paintings/kandinsky/",
    "d:/paintings/klee/",
    "d:/paintings/klimt/",
    "d:/paintings/kooning/",
    "d:/paintings/leger/",
    "d:/paintings/lichtenstein/",
    "d:/paintings/lissitzky/",
    "d:/paintings/lorrain/",
    "d:/paintings/magritte/",
    "d:/paintings/malevich/",
    "d:/paintings/manet/",
    "d:/paintings/mantegna/",
    "d:/paintings/marc/",
    "d:/paintings/masaccio/",
    "d:/paintings/memling/",
    "d:/paintings/michelangelo/",
    "d:/paintings/millet/",
    "d:/paintings/miro/",
    "d:/paintings/modigliani/",
    "d:/paintings/mondrian/",
    "d:/paintings/monet/",
    "d:/paintings/moreau/",
    "d:/paintings/munch/",
    "d:/paintings/okeeffe/",
    "d:/paintings/patinir/",
    "d:/paintings/picasso/",
    "d:/paintings/pissarro/",
    "d:/paintings/pollock/",
    "d:/paintings/poussin/",
    "d:/paintings/raffael/",
    "d:/paintings/rembrandt/",
    "d:/paintings/renoir/",
    "d:/paintings/richter/",
    "d:/paintings/rijn/",
    "d:/paintings/rivera/",
    "d:/paintings/rossetti/",
    "d:/paintings/rubens/",
    "d:/paintings/sargent/",
    "d:/paintings/schiele/",
    "d:/paintings/seurat/",
    "d:/paintings/tintoretto/",
    "d:/paintings/tizian/",
};

const char *ArtCritic::test_painters_[] = {
    "d:/paintings/turner/",
    "d:/paintings/uccello/",
    "d:/paintings/van_gogh/",
    "d:/paintings/velazquez/",
    "d:/paintings/vermeer/",
    "d:/paintings/warhol/",
    "d:/paintings/watteau/",
    "d:/paintings/weyden/",
    "d:/paintings/whistler/",
    "d:/paintings/zurbaran/",
};

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

    int num_train_painters = sizeof(train_painters_) / sizeof(const char *);
    for (int i = 0; i < num_train_painters; i++) {
        feature_creator_->WriteFeatureFile(train_painters_[i], fid, +1);
    }

    fclose(fid);
    return 0;
}

int main(int argc, char *argv[]) {
    ArtCritic critic;

    critic.CreatePositiveTrainFile("../data/positive_train.txt");

    return 0;
}
