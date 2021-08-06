#include "Music.h"

#include <iostream>
#include <sstream>

DWORD WINAPI MusicThreadFunc(LPVOID lpParameter)
{
    Music* music = static_cast<Music*>(lpParameter);

    while (1)
    {
        // Try to unprepare header
        if (waveOutUnprepareHeader(music->wave_out_,
                                   &(music->wave_header_[music->next_play_block_]),
                                   sizeof(WAVEHDR))
            != WAVERR_STILLPLAYING)
        {
            music->FillNextBlock();
            waveOutPrepareHeader(music->wave_out_,
                                 &(music->wave_header_[music->next_play_block_]),
                                 sizeof(WAVEHDR));
            waveOutWrite(music->wave_out_,
                         &(music->wave_header_[music->next_play_block_]),
                         sizeof(WAVEHDR));
            music->NextPlayBlock();
        }
        else
        {
            Sleep(1);
        }
    }

    return 0;
}


void ByteBeat::Compute(int start_time, int num_samples, unsigned char* output)
{
    for (int sample = 0; sample < num_samples; sample++)
    {
        int stack_pos = 0;
        int t = sample + start_time;
        for (int c = 0; c < code_length_; c++)
        {
            int code = code_[c];
            int value;
            switch (code)
            {
            case MUSIC_CODE_PUSH:
                c++;
                stack_[stack_pos++] = code_[c];
                break;
            case MUSIC_CODE_PUSH_T:
                stack_[stack_pos++] = t;
                break;
            case MUSIC_CODE_ADD:
                value = stack_[stack_pos - 2] + stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            case MUSIC_CODE_MUL:
                value = stack_[stack_pos - 2] * stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            case MUSIC_CODE_AND:
                value = stack_[stack_pos - 2] & stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            case MUSIC_CODE_OR:
                value = stack_[stack_pos - 2] | stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            case MUSIC_CODE_RIGHT_SHIFT:
                value = stack_[stack_pos - 2] >> stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            case MUSIC_CODE_LEFT_SHIFT:
                value = stack_[stack_pos - 2] >> stack_[stack_pos - 1];
                stack_[stack_pos - 2] = value;
                stack_pos--;
                break;
            default:
                break;
            }
        }

        if (stack_pos == 1)
        {
            output[sample] = static_cast<unsigned char>(stack_[0]);
        }
        else
        {
            output[sample] = 0;
        }
    }
}

void ByteBeat::WriteStackTrace(std::string &parsed, int stack_size = 1)
{
    // Write stack trace
    parsed = "";
    std::ostringstream output_stream;
    for (int i = 0; i < code_length_; i++)
    {
        switch (code_[i])
        {
        case MUSIC_CODE_PUSH_T:
            output_stream << "T ";
            break;
        case MUSIC_CODE_ADD:
            output_stream << "+ ";
            break;
        case MUSIC_CODE_MUL:
            output_stream << "* ";
            break;
        case MUSIC_CODE_AND:
            output_stream << "& ";
            break;
        case MUSIC_CODE_OR:
            output_stream << "| ";
            break;
        case MUSIC_CODE_RIGHT_SHIFT:
            output_stream << "> ";
            break;
        case MUSIC_CODE_LEFT_SHIFT:
            output_stream << "< ";
            break;
        case MUSIC_CODE_PUSH:
            output_stream << code_[i + 1] << " ";
            i++;
            break;
        default:
            output_stream << "[UNKNOWN_CODE] ";
            break;
        }
    }
    output_stream << "(stack: " << stack_size << ")";
    parsed = output_stream.str();
}

bool ByteBeat::Parse(const std::string &text, std::string &parsed)
{
    int stack_size = 0;
    code_length_ = 0;
    std::istringstream input_stream(text);
    std::string token;

    while (std::getline(input_stream, token, ' '))
    {
        switch (token[0])
        {
        case 't':
        case 'T':
            code_[code_length_++] = MUSIC_CODE_PUSH_T;
            stack_size++;
            break;
        case '+':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_ADD;
            stack_size--;
            break;
        case '*':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_MUL;
            stack_size--;
            break;
        case '&':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_AND;
            stack_size--;
            break;
        case '|':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_OR;
            stack_size--;
            break;
        case '>':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_RIGHT_SHIFT;
            stack_size--;
            break;
        case '<':
            if (stack_size < 2) { code_length_ = 0; stack_size = -1; break; }
            code_[code_length_++] = MUSIC_CODE_LEFT_SHIFT;
            stack_size--;
            break;
        default:
            code_[code_length_++] = MUSIC_CODE_PUSH;
            code_[code_length_++] = atoi(token.c_str());
            stack_size++;
            break;
        }
    }

    WriteStackTrace(parsed, stack_size);

    if (stack_size != 1) { code_length_ = 0; return false; }
    else return true;
}

WAVEFORMATEX Music::WFX = {
    WAVE_FORMAT_PCM,		                // wFormatTag
    MUSIC_NUM_CHANNELS,                     // nChannels
    MUSIC_RATE,   			                // nSamplesPerSec
    MUSIC_RATE * MUSIC_NUM_CHANNELS * 2, 	// nAvgBytesPerSec
    MUSIC_NUM_CHANNELS * 2,			        // nBlockAlign
    16,	            					    // wBitsPerSample
    0           						    // cbSize
};

Music::Music(void)
{
    if (waveOutOpen(&wave_out_, WAVE_MAPPER, &WFX, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        throw;
    }

    // prepare and play music block
    for (int i = 0; i < MUSIC_NUM_PLAY_BLOCKS; i++)
    {
        for (int j = 0; j < MUSIC_BUFFER_SIZE; j++)
        {
            audio_block_[i][j] = 0;
        }

        wave_header_[i].lpData = (char*)audio_block_[i];
        wave_header_[i].dwBufferLength = MUSIC_BUFFER_SIZE * MUSIC_NUM_CHANNELS * 2;
        waveOutPrepareHeader(wave_out_, &(wave_header_[i]), sizeof(WAVEHDR));
        waveOutWrite(wave_out_, &(wave_header_[i]), sizeof(WAVEHDR));
    }

    CreateThread(NULL, 0, MusicThreadFunc, NULL, 0, 0);
}

Music::~Music(void)
{
    waveOutClose(wave_out_);
}

void Music::FillNextBlock(void)
{
}