#pragma once

#include <Windows.h>
#include <mmeapi.h>

#include "Error.h"

// This is one hack that doesn't un-initialize the microphone...

class Microphone
{
public:
    Microphone();
    virtual ~Microphone();

    bool Init(char *error_buffer);
    bool GetAmplitude(float *amplitude, char *error_buffer);

private:
    bool initialized = false;
    HWAVEIN hWaveIn_ = 0;
    WAVEHDR WaveHdr_;
    float music_loudness_ = 0.0f;
    short *wave_data_ = nullptr;
    int num_samples_;

    static constexpr float kMilliSeconds = 100;
};

