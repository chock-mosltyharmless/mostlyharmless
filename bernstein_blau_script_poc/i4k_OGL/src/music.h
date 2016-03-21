#pragma once

// This is the music data file.
#define NUM_INSTURMENT_PARAMS K_MASTER_PANNING

// Here I have no music data...
#include "music/music.353898560.txt"
#include "music/music.419086272.txt"

unsigned char *instrumentParams[] = {
    instrumentParams_353898560,
    instrumentParams_419086272
};

unsigned short *savedNoteTime[] = {
    savedNoteTime_353898560,
    savedNoteTime_419086272
};

signed char *savedNote[] = {
    savedNote_353898560,
    savedNote_419086272
};