// clear audio block
for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++) {
    // Stereo! --> 2 values
    floatOutput[0][sample] = 0;
}

//int on[18] = {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

for (int instrument = 0; instrument < NUM_INSTRUMENTS; instrument++)
{
    int note_pos = savedNotePos[instrument];
    //if (!on[instrument])continue;
    //if (instrument != 4) continue;

    // Volume?
    // Get parameters locally
    float vol = float_instrument_parameters_[instrument][F_MASTER_VOLUME];
    //float panning = float_instrument_parameters_[instrument][K_MASTER_PANNING];
    float panning = 0.785f - instrument * (1.0f / 24.0f);
    float invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);

    // Check if we go to next note
    if (savedNoteTime__[note_pos /*+ currentNoteIndex[instrument]*/] == 0) {
        if (savedNote__[note_pos /*+ currentNoteIndex[instrument]*/] != -128) {
            // Key on
            iADSR[instrument] = 0; // attack
            fADSRVal[instrument] = 0.0f; // starting up

            for (int i = 0; i < NUM_ADSR_DATA; i++) {
                adsrData[instrument][i] = float_instrument_parameters_[instrument][i * 4 + iADSR[instrument]];
            }

            // Apply delta-note
            currentNote[instrument] += savedNote__[note_pos /*+ currentNoteIndex[instrument]*/];
            i_midi_volume_[instrument] += savedVelocity__[/*currentNoteIndex[instrument] +*/ velocityPos[instrument]];

            // Set the oscillator phases to zero
            fPhase[instrument] = 0.f;
        }
        else {
            // NoteOff
            iADSR[instrument] = 2; // Release
        }

        invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);

        // Go to next note location
        //currentNoteIndex[instrument]++;
        velocityPos[instrument]++;
        note_pos++;
    }
    savedNoteTime__[note_pos /*+ currentNoteIndex[instrument]*/]--;

    // ignore everything before the first note
    //if (currentNoteIndex[instrument] == 0) {
    //    continue;
    //}

    if (float_instrument_parameters_[instrument][K_VOLUME + iADSR[instrument] + 1] < (1.0f / 1024.0f) &&
        adsrData[instrument][adsrVolume] < (1.0f / 1024.0f)) continue;

    // Get audio frequency
    float baseFreq = 8.175f * (float)exp2jo((float)currentNote[instrument] * (1.0f / 12.0f)) * fScaler;

    float base_phase = fPhase[instrument];
    float phase_update = baseFreq * (adsrData[instrument][adsrFreq] * 4.0f);
    for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++) {
        float deathVolume = 1.0f;

        // Process ADSR envelope
        if (iADSR[instrument] == 0) {
            fADSRVal[instrument] += (1.0f - fADSRVal[instrument]) *
                (float_instrument_parameters_[instrument][K_ADSR_SPEED + 0] * (1.0f / 1024.0f));
        }
        // Go from attack to decay
        if (iADSR[instrument] == 0 && fADSRVal[instrument] > 0.75f) {
            iADSR[instrument] = 1;
            invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);
        }

        // interpolate volume according to ADSR envelope
        for (int i = 0; i < NUM_ADSR_DATA; i++) {
            adsrData[instrument][i] += (float_instrument_parameters_[instrument][i * 4 + iADSR[instrument] + 1] - adsrData[instrument][i]) * invADSRSpeed;
        }

        if (savedNoteTime__[note_pos /*+ currentNoteIndex[instrument]*/] == 0 &&
            sample >= MZK_BLOCK_SIZE - 1024 &&
            savedNote__[note_pos /*+ currentNoteIndex[instrument]*/] != -128) {
            deathVolume = (MZK_BLOCK_SIZE - sample) * (1.0f / 1024.0f);
        }

        float outAmplitude = 0;

        float overtoneLoudness = 1.0f;
        float phase = base_phase;
        float overtone_falloff = adsrData[instrument][adsrQuak] * 2.0f;
        for (int i = 0; i < NUM_OVERTONES && (i + 1) * phase_update < 2.0f; i++) {
            outAmplitude += sinf(phase) * overtoneLoudness *
                (1.0f + (lowNoise[(sample + i * 4096) % RANDOM_BUFFER_SIZE] - 1.0f) * adsrData[instrument][adsrNoise]);
            phase += base_phase;
            // Ring modulation with noise
            //outAmplitude *= 1.0f + (lowNoise[sample % RANDOM_BUFFER_SIZE] - 1.0f) * adsrData[instrument][adsrNoise];
            overtoneLoudness *= overtone_falloff;
        }

        base_phase += phase_update;
        while (base_phase > 2.0f * PI) base_phase -= 2.0f * (float)PI;

        // Ring modulation with noise
        float cur_vol = vol * adsrData[instrument][adsrVolume] * deathVolume * i_midi_volume_[instrument] * (1.0f / 128.0f);
        float distortMult = (float)exp2jo(8.0f*adsrData[instrument][adsrDistort]) + 8.0f;
        float current_pan = panning;
        for (int i = 0; i < 2; i++) {
            float output = outAmplitude * current_pan;

            // Distort
            output *= distortMult;
#if 1
            //output = 2.0f * (1.0f / (1.0f + (float)exp2jo(-2.0f * output)) - 0.5f);
            output = output / (fabsf(output) + 1.0f);
#else
            //if (output > 1.0f) output = 1.0f;
            //if (output < -1.0f) output = -1.0f;
#endif
            output /= distortMult;
            floatOutput[sample][i] += output * cur_vol;

            // Apply stereo
            current_pan = 1.0f - current_pan;
        }
    }
    fPhase[instrument] = base_phase;
    savedNotePos[instrument] = note_pos;
}

// Copy to int output
for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
{
#if 1
    //float val = -8.0f * floatOutput[0][sample];
    //val = 2.0f * 32768.0f * (1.0f / (1.0f + (float)exp2jo(val)) - 0.5f);
    float val = 7.0f * floatOutput[0][sample];
    val = 2.0f * val / (fabsf(val) + 1.0f);
    val *= 32767.0f;
#else
    float val = 4.0f * 32768.0f * floatOutput[0][sample];
    if (val > 32767.0f) val = 32767.0f;
    if (val < -32767.0f) val = -32767.0f;
#endif
    blockBuffer[sample] = _mm_cvtt_ss2si(_mm_load_ss(&val));
}

#ifdef _DEBUG
FILE *fid = fopen("waveout.raw", "ab");
fwrite(blockBuffer, sizeof(short), MZK_BLOCK_SIZE * 2, fid);
fclose(fid);
#endif
