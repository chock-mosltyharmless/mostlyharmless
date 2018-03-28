//float (*reverb_buffer_)[2] = reverb_buffer_ + reverb_pos_;

// clear audio block
for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++) {
    // Stereo! --> 2 values
    reverb_buffer_[0][sample] = 0;
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
    //float panning = 0.785f - instrument * (1.0f / 24.0f);
    float panning = (float)((instrument & 7) + 12) * (1.0f / 32.0f);
    //float panning = (float)((instrument & 7)) * (1.0f / 8.0f);
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

        float overtone_falloff = adsrData[instrument][adsrQuak] * 2.0f;
        float noise_modulator = 1.0f + (lowNoise[sample % RANDOM_BUFFER_SIZE] - 1.0f) * adsrData[instrument][adsrNoise];
        float cur_vol = vol * adsrData[instrument][adsrVolume] * deathVolume * i_midi_volume_[instrument] * (1.0f / 128.0f);
        float distortMult = (float)exp2jo(8.0f*adsrData[instrument][adsrDistort]) + 8.0f;

#if 1
        for (int supersample = 0; supersample < 2; supersample++) {
            float phase = base_phase;
            float overtoneLoudness = 1.0f;
            float sub_amplitude = 0.0f;
            for (int i = 0; i < NUM_OVERTONES; i++) {
                sub_amplitude += sinf(phase) * overtoneLoudness;
                phase += base_phase;
                overtoneLoudness *= overtone_falloff;
                // Ring modulation with noise
                sub_amplitude *= noise_modulator;
            }

            base_phase += baseFreq * (adsrData[instrument][adsrFreq] * 2.0f);

            // Distort
            sub_amplitude *= distortMult;
            sub_amplitude = 2.0f * (1.0f / (1.0f + (float)exp2jo(2.0f * sub_amplitude)) - 0.5f);
            sub_amplitude /= distortMult;
            outAmplitude += sub_amplitude;
        }
#else
        float phase = base_phase;
        float overtoneLoudness = 1.0f;
        for (int i = 0; i < NUM_OVERTONES; i++) {
            outAmplitude += sinf(phase) * overtoneLoudness;
            phase += base_phase;
            overtoneLoudness *= overtone_falloff;
            // Ring modulation with noise
            outAmplitude *= noise_modulator;
        }

        base_phase += baseFreq * (adsrData[instrument][adsrFreq] * 4.0f);

        // Distort
        outAmplitude *= distortMult;
        outAmplitude = 2.0f * (1.0f / (1.0f + (float)exp2jo(2.0f * outAmplitude)) - 0.5f);
        outAmplitude /= distortMult;
#endif

        reverb_buffer_[sample][0] += outAmplitude * panning * cur_vol;
        reverb_buffer_[sample][1] += outAmplitude * (1.0f - panning) * cur_vol;

        while (base_phase > 2.0f * PI) base_phase -= 2.0f * (float)PI;
    }
    fPhase[instrument] = base_phase;
    savedNotePos[instrument] = note_pos;
}

#if 0
// Put everything into the reverb buffers
for (int sample_id = 0; sample_id < MZK_BLOCK_SIZE; sample_id++) {
    for (int j = 0; j < NUM_STEREO_VOICES; j++) {
        // Do the reverb feedback
        const float kDelayFeed = 0.325f;
        int toBuffer = j & 1;
        int fromBuffer = 1 - toBuffer;
        int fromLocation = (sample_id + MZK_BLOCK_SIZE - reverbBufferLength[j]);
        reverb_buffer_[sample_id][toBuffer] +=
            kDelayFeed * reverb_buffer_[fromLocation % MZK_BLOCK_SIZE][fromBuffer];
    }
}
#endif

// Copy to int output
for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
{
#if 1
#if 0
    float val = -8.0f * floatOutput[0][sample];
    val = 2.0f * 32768.0f * (1.0f / (1.0f + (float)exp2jo(val)) - 0.5f);
#else
    float val = 5.0f * reverb_buffer_[0][sample];
    //val = 2.0f * val / (fabsf(val) + 1.0f);
    if (val > 1.0f) val = 1.0f;
    if (val < -1.0f) val = -1.0f;
    val = 3.0f * val / 2.0f * (1.0f - val * val / 3.0f);
    val *= 32767.0f;
#endif
#else
    float val = 4.0f * 32768.0f * floatOutput[0][sample];
    if (val > 32767.0f) val = 32767.0f;
    if (val < -32767.0f) val = -32767.0f;
#endif
    blockBuffer[sample] = _mm_cvtt_ss2si(_mm_load_ss(&val));
}

//reverb_pos_ += MZK_BLOCK_SIZE;
//reverb_pos_ = reverb_pos_ % REVERB_BUFFER_SIZE;

#ifdef _DEBUG
FILE *fid = fopen("waveout.raw", "ab");
fwrite(blockBuffer, sizeof(short), MZK_BLOCK_SIZE * 2, fid);
fclose(fid);
#endif
