#include "Microphone.h"

#include <math.h>

Microphone::Microphone()
{
}


Microphone::~Microphone()
{
    if (wave_data_) delete [] wave_data_;
}

bool Microphone::Init(char * error_buffer)
{
    ErrorPrint(error_buffer, "");

    // Initialize Wave input
    WAVEFORMATEX WaveFormat;
    MMRESULT MMResult;
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
    WaveFormat.nChannels = 1;                    // 1=mono, 2=stereo
    WaveFormat.nSamplesPerSec = 44100;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign =
        WaveFormat.nChannels*(WaveFormat.wBitsPerSample / 8);
    WaveFormat.nAvgBytesPerSec =
        WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
    WaveFormat.cbSize = 0;
    num_samples_ = (int)(WaveFormat.nSamplesPerSec * kMilliSeconds / 1000.0f);
    WaveHdr_.dwBufferLength =
        num_samples_ * (WaveFormat.wBitsPerSample / 8) * WaveFormat.nChannels;
    WaveHdr_.dwBytesRecorded = 0L;
    WaveHdr_.dwUser = 0L;
    WaveHdr_.dwFlags = 0L;
    WaveHdr_.dwLoops = 0L;
    wave_data_ = new short[WaveHdr_.dwBufferLength / sizeof(short)];
    WaveHdr_.lpData = (char *)wave_data_;
    // open device
    MMResult = waveInOpen(&hWaveIn_, WAVE_MAPPER, &WaveFormat, 0, 0, CALLBACK_NULL);
    // prepare header
    MMResult = waveInPrepareHeader(hWaveIn_, &WaveHdr_, sizeof(WAVEHDR));
    // send buffer to Windows
    MMResult = waveInAddBuffer(hWaveIn_, &WaveHdr_, sizeof(WAVEHDR));
    // start input
    MMResult = waveInStart(hWaveIn_);
    float music_loudness = 0.0f;
    initialized = true;

    return true;
}

bool Microphone::GetAmplitude(float * amplitude, char * error_buffer)
{
    ErrorPrint(error_buffer, "");

    if (!initialized)
    {
        ErrorPrint(error_buffer, "Microphone not initialized");
        return false;
    }

    // Get Music:
    if (waveInUnprepareHeader(hWaveIn_, &WaveHdr_, sizeof(WAVEHDR)) != WAVERR_STILLPLAYING)
    {
        MMRESULT MMResult;
        double cumulative_square = 0.0f;
        for (int i = 0; i < num_samples_; i++) {
            cumulative_square += (double)wave_data_[i] * (double)wave_data_[i];
        }
        if (cumulative_square < 1.0f) cumulative_square = 1.0f;
        music_loudness_ = (float)(log(cumulative_square));
        MMResult = waveInStop(hWaveIn_);
        MMResult = waveInPrepareHeader(hWaveIn_, &WaveHdr_, sizeof(WAVEHDR));
        MMResult = waveInAddBuffer(hWaveIn_, &WaveHdr_, sizeof(WAVEHDR));
        MMResult = waveInStart(hWaveIn_);
    }

    *amplitude = music_loudness_;

    return true;
}

