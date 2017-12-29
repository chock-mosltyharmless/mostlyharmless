#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>

// Interactive Multi-channel audio mixer for mono audio at fixed 48000 Hz

#define AU_DIRECTORY "waves/"
// Note that practically no audio files are supported
#define AU_WAVE_WILDCARD "*.wav"
#define AU_SAMPLING_RATE 48000
#define AU_NUM_CHANNELS 6
#define AU_MAX_NUM_WAVES 128
#define AU_MAX_FILENAME_LENGTH 1024
#define AU_FADE_IN_START_DB -80.0f
// Max. 1 hour audio files
#define AU_MAX_AUDIO_LENGTH (AU_SAMPLING_RATE * 60 * 60)
#define AU_NUM_PLAY_BLOCKS 4
#define AU_AUDIO_BUFFER_SIZE 2048

// TODO:
// I need a semaphore that blocks audio rendering and modifications.
// Everything else is too hot.

class Audio
{
public:
    Audio();
    ~Audio();

    // Load all wave files from the ./waves directory.
    // Returns 0 if successful, -1 otherwise
    // The error string must hold space for at least SM_MAX_ERROR_LENGTH
    // characters. It contains errors from compilation/loading
    // This also starts an audio thread that starts streaming silence
    int Init(char *error_string);

    // Plays a wave file on a given channel. If anything else is playing in the channel,
    // it is overwritten. channel must be in the range 0..AU_NUM_CHANNELS-1
    // loop does not cross-fade, so make sure that your audio loops correctly.
    // fade_in is in dB per second, a negative number means instant
    int PlaySound(const char *name, int channel, bool loop, float fade_in, char *error_string,
        float volume = 1.0f);

    // Stops whatever is playing in the given channel. May be called multiple times without
    // PlaySound inbetween, this has no effect unless fade_out speed is changed
    // fade_out is in dB per second.
    int StopSound(int channel, float fade_out, char *error_string);

    // Get time in audio domain
    float GetTime(void) {
        MMTIME timer; // Using getPosition of wave audio playback
        timer.wType = TIME_SAMPLES;
        waveOutGetPosition(hWaveOut, &timer, sizeof(timer));
        DWORD t = timer.u.sample;
        return (float)t / (float)AU_SAMPLING_RATE;
    }

private:
    int LoadWave(const char *filename, char *errorString);
    // Does not clip and does not convert to integer.
    void RenderSamples(float *mix, int num_samples);
    void ReleaseAll(void);
    static DWORD WINAPI Audio::ThreadFunc(LPVOID lpParameter);

private:
    // static audio data
    int num_waves_;
    char wave_name_[AU_MAX_NUM_WAVES][AU_MAX_FILENAME_LENGTH + 1];
    // allocated for num_waves_, in range -1..1
    float *wave_data_[AU_MAX_NUM_WAVES];
    int wave_length_[AU_MAX_NUM_WAVES];

    // Dynamic channel information
    int channel_wave_id_[AU_NUM_CHANNELS];  // -1 if nothing is playing
    int channel_position_[AU_NUM_CHANNELS];  // current position in the wave file
    float channel_volume_[AU_NUM_CHANNELS];  // for fade in/out
    float channel_fade_in_[AU_NUM_CHANNELS];  // in dB / s, negative if no in-fading
    float channel_fade_out_[AU_NUM_CHANNELS];  // in dB / s, negative if no out-fading
    bool channel_loop_[AU_NUM_CHANNELS];
    float channel_max_volume_[AU_NUM_CHANNELS];

    // Audio playback data
    // Audio playback stuff
    bool wave_out_prepared_;
    HWAVEOUT hWaveOut; // audio device handle
                              // TODO: Use more than 2 buffers with very small sizes so that I have
                              //       no influence on frame rate!
    int nextPlayBlock = 0; // The block that must be filled and played next
    short myMuzikBlock[AU_NUM_PLAY_BLOCKS][AU_AUDIO_BUFFER_SIZE]; // The audio blocks
    WAVEHDR header[AU_NUM_PLAY_BLOCKS];    // header of the audio block
    static const WAVEFORMATEX wfx;
};

