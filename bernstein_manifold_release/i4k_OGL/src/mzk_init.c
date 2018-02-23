// Create data tables

// Make table with random number
//unsigned int seed = 1;
for (int i = 0; i < RANDOM_BUFFER_SIZE; i++) {
    lowNoise[i] = 16.0f * (JoFrand() - 0.5f);
}

// Ring-low-pass-filtering of lowPass
// Use a one-pole
float oldVal = 0.0f;
for (int j = 0; j < 6; j++) {
    for (int i = 0; i < RANDOM_BUFFER_SIZE; i++) {
        lowNoise[i] = 1.0f / 8.0f * lowNoise[i] + (1.0f - (1.0f / 8.0f)) * oldVal;
        oldVal = lowNoise[i];
    }
}

// Convert int parameters to float parameters
for (int inst = 0; inst < NUM_INSTRUMENTS; inst++) {
    for (int param = 0; param < NUM_INSTRUMENT_PARAMETERS; param++) {
        float_instrument_parameters_[inst][param] = (float)instrumentParams[inst][param] * (1.0f / 128.0f);
    }

    // Delta-uncompress note locations
    savedNotePos[inst + 1] += savedNotePos[inst];
}

// Convert byte note time to short note time
unsigned char *read = savedNoteTime___;
signed short *write = savedNoteTime__;
while (*read != 254) {
    *write += *read;
    if (*read < 255) *write++;
    read++;
}