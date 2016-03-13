#include "stdafx.h"
#include "Audio.h"
#include "Configuration.h"

const WAVEFORMATEX Audio::wfx = {
    WAVE_FORMAT_PCM,					// wFormatTag
    1,				                	// nChannels
    AU_SAMPLING_RATE,					// nSamplesPerSec
    AU_SAMPLING_RATE * 2,	       		// nAvgBytesPerSec
    1 * 2,				                // nBlockAlign
    16,									// wBitsPerSample
    0									// cbSize
};

Audio::Audio() {
    num_waves_ = 0;
    wave_out_prepared_ = false;
    ReleaseAll();
}

Audio::~Audio() {
    ReleaseAll();
}

DWORD WINAPI Audio::ThreadFunc(LPVOID lpParameter) {
    Audio *audio = (Audio *)lpParameter;

    float *float_data = new float[AU_AUDIO_BUFFER_SIZE];

    while (audio->num_waves_ > 0 && audio->wave_out_prepared_) {
        // Try to unprepare header
        if (waveOutUnprepareHeader(audio->hWaveOut, &(audio->header[audio->nextPlayBlock]), sizeof(WAVEHDR))
            != WAVERR_STILLPLAYING) {
            int block = audio->nextPlayBlock;
            audio->RenderSamples(float_data, AU_AUDIO_BUFFER_SIZE);
            for (int i = 0; i < AU_AUDIO_BUFFER_SIZE; i++) {
                int sample = (int)(float_data[i] * 32768);
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                audio->myMuzikBlock[block][i] = sample;
            }
            waveOutPrepareHeader(audio->hWaveOut, &(audio->header[block]), sizeof(WAVEHDR));
            waveOutWrite(audio->hWaveOut, &(audio->header[block]), sizeof(WAVEHDR));
            audio->nextPlayBlock++;
            if (audio->nextPlayBlock >= AU_NUM_PLAY_BLOCKS) audio->nextPlayBlock = 0;
        } else {
            Sleep(1);
        }
    }

    // This will not work, but who cares?
    delete[] float_data;

    return 0;
}

int Audio::Init(char * error_string) {
    ReleaseAll();

    // Go throught the waves directory and load all audio files.
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;

    // Go to first file in textures directory
    char *dirname = AU_DIRECTORY AU_WAVE_WILDCARD;
    hFind = FindFirstFile(AU_DIRECTORY AU_WAVE_WILDCARD, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        sprintf_s(error_string, MAX_ERROR_LENGTH,
            "IO Error\nThere are no wave files in " AU_DIRECTORY);
        return -1;
    }

    // Load all the textures in the directory
    do {
        // Note that the number of textures is increased automatically
        int ret_val = LoadWave(ffd.cFileName, error_string);
        if (ret_val) return ret_val;
    } while (FindNextFile(hFind, &ffd));

    // Initialize audio device
    // open audio device
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    // open audio device
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx,
        0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "unable to open WAVE_MAPPER device");
        return -1;
    }
    wave_out_prepared_ = true;

    // prepare and play music block
    for (int i = 0; i < AU_NUM_PLAY_BLOCKS; i++) {
        header[i].lpData = (char *)myMuzikBlock[i];
        header[i].dwBufferLength = AU_AUDIO_BUFFER_SIZE * 2;
        waveOutPrepareHeader(hWaveOut, &(header[i]), sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &(header[i]), sizeof(WAVEHDR));
    }

    CreateThread(NULL, 0, ThreadFunc, this, 0, 0);

    return 0;
}

int Audio::PlaySound(const char *name, int channel, bool loop, float fade_in, char *error_string) {
    if (channel < 0 || channel >= AU_NUM_CHANNELS) {
        sprintf_s(error_string, MAX_ERROR_LENGTH,
            "Channel %d not supported", channel);
        return -1;
    }

    // Find the ID of the name
    int audio_id = -1;
    for (int i = 0; i < num_waves_; i++) {
        if (strcmp(name, wave_name_[i]) == 0) audio_id = i;
    }
    if (audio_id < 0) {
        sprintf_s(error_string, MAX_ERROR_LENGTH,
            "Wave file '%s' not found.", name);
        return -1;
    }

    channel_wave_id_[channel] = audio_id;
    channel_position_[channel] = 0;
    channel_loop_[channel] = loop;
    channel_fade_out_[channel] = -1.0f;
    if (fade_in < 0.0f) {
        channel_volume_[channel] = 1.0f;
        channel_fade_in_[channel] = -1.0f;
    } else {
        // start sound with -70dB
        channel_volume_[channel] = expf(-70.0f / 6.0f * logf(2.0f));
        channel_fade_in_[channel] = fade_in;
    }

    return 0;
}

int Audio::StopSound(int channel, float fade_out, char *error_string) {
    if (channel < 0 || channel >= AU_NUM_CHANNELS) {
        sprintf_s(error_string, MAX_ERROR_LENGTH,
            "Channel %d not supported", channel);
        return -1;
    }

    channel_fade_in_[channel] = -1.0f;
    if (fade_out < 0.0f) {
        channel_volume_[channel] = expf(AU_FADE_IN_START_DB / 6.0f * logf(2.0f));
    } else {
        channel_fade_out_[channel] = fade_out;
    }
    return 0;
}

int Audio::LoadWave(const char * filename, char * error_string) {
    char combined_name[AU_MAX_FILENAME_LENGTH + 1];

    sprintf_s(combined_name, AU_MAX_FILENAME_LENGTH,
        AU_DIRECTORY "%s", filename);

    if (num_waves_ >= AU_MAX_NUM_WAVES) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "Too many wave files.");
        return -1;
    }

    FILE *fid;
    if (fopen_s(&fid, combined_name, "rb")) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "Could not find wave '%s'",
            filename);
        return -1;
    }

    // Load header (best guess)
    int header[11];
    if (fread(header, sizeof(int), 11, fid) < 10) {
        fclose(fid);
        sprintf_s(error_string, MAX_ERROR_LENGTH, "Invalid wave file '%s'",
            filename);
        return -1;
    }

    // Stupid header checking
    if (header[4] != 16) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "%s: Not 16-bit audio", filename);
        return -1;
    }
    if (header[6] != AU_SAMPLING_RATE) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "%s: Sampling rate is not 48000 Hz", filename);
        return -1;
    }
    bool stereo = false;
    if (header[7] != 2 * AU_SAMPLING_RATE) {
        // This is how I know it is stereo...
        if (header[7] == 4 * AU_SAMPLING_RATE) {
            stereo = true;
        } else {
            sprintf_s(error_string, MAX_ERROR_LENGTH, "%s: Unknown format", filename);
            return -1;
        }
    }
    // TODO: Number of channels [I could just take the left one for stereo...

    int num_samples = header[10] / 2;
    wave_length_[num_waves_] = num_samples;
    if (stereo) wave_length_[num_waves_] = num_samples / 2;
    if (wave_length_[num_waves_] > AU_MAX_AUDIO_LENGTH) {
        sprintf_s(error_string, MAX_ERROR_LENGTH, "%s: Too many samples: %d",
            filename, wave_length_[num_waves_]);
        return -1;
    }
    short *short_buffer = new short[num_samples];
    
    if (fread(short_buffer, sizeof(short), num_samples, fid) !=
        num_samples) {
        delete[] short_buffer;
        sprintf_s(error_string, MAX_ERROR_LENGTH, "%s: Could not load samples for some reason...", filename);
        return -1;
    }

    wave_data_[num_waves_] = new float[wave_length_[num_waves_]];
    for (int i = 0; i < wave_length_[num_waves_]; i++) {
        if (stereo) wave_data_[num_waves_][i] = (float)(short_buffer[i * 2]) / 32768.0f;
        else wave_data_[num_waves_][i] = (float)(short_buffer[i]) / 32768.0f;
    }

    strcpy_s(wave_name_[num_waves_], AU_MAX_FILENAME_LENGTH, filename);

    num_waves_++;

    delete[] short_buffer;
    fclose(fid);

    return 0;
}

void Audio::RenderSamples(float *mix, int num_samples) {
    // Clear the mix so that the following calls can just add
    for (int sample = 0; sample < num_samples; sample++) {
        mix[sample] = 0.0f;
    }
    
    for (int channel = 0; channel < AU_NUM_CHANNELS; channel++) {
        int wave_id = channel_wave_id_[channel];
        if (wave_id >= 0) {
            int position = channel_position_[channel];
            int length = wave_length_[wave_id];
            bool loop = channel_loop_[channel];
            float volume = channel_volume_[channel];
            float fade_in = channel_fade_in_[channel];
            float fade_out = channel_fade_out_[channel];
            float fade_multiply = 1.0f;
            const float *wave = wave_data_[wave_id];
            if (fade_in >= 0.0f) {
                // fade_in of 6 equals fade_multiply of 2.0f
                fade_multiply = expf(fade_in / 6.0f / AU_SAMPLING_RATE * logf(2.0f));
            }
            // Fade out has higher priority
            if (fade_out >= 0.0f) {
                // fade_out of 6 equals fade_multiply of 2.0f
                fade_multiply = expf(-fade_out / 6.0f / AU_SAMPLING_RATE * logf(2.0f));
            }

            for (int sample = 0; sample < num_samples; sample++) {
                position++;
                if (position >= length) {
                    position = 0;
                    if (!loop) {
                        volume = 0.0f;
                        fade_in = -1.0f;
                        fade_out = -1.0f;
                    }
                }
                volume *= fade_multiply;
                if (volume > 1.0f) volume = 1.0f;
                if (volume < 1e-10f) volume = 0.0f;  // Avoid denormal floats

                mix[sample] += volume * wave[position];
            }

            // Write back modified channel data
            channel_position_[channel] = position;
            channel_volume_[channel] = volume;
        }
    }
}

void Audio::ReleaseAll(void) {
    int num = num_waves_;
    num_waves_ = 0;
    for (int i = 0; i < AU_NUM_CHANNELS; i++) {
        channel_wave_id_[i] = -1;
    }
    
    // Assume that playing stops after a while
    Sleep(200);
    for (int i = 0; i < num_waves_; i++) {
        delete[] wave_data_[i];
    }
    if (wave_out_prepared_) {
        waveOutClose(hWaveOut);
    }
}
