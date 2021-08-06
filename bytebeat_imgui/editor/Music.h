#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>

#include <string>

#define MUSIC_MAX_STACK_SIZE 1024
#define MUSIC_MAX_CODE_SIZE 4096

class ByteBeat
{
public:
	void Compute(int start_time, int num_samples, unsigned char* output);
	bool Parse(const std::string &text, std::string &parsed);

private:
    void ByteBeat::WriteStackTrace(std::string& parsed, int stack_size);

	static constexpr int MUSIC_CODE_PUSH = 1;
	static constexpr int MUSIC_CODE_PUSH_T = 2;
	static constexpr int MUSIC_CODE_ADD = 3;
	static constexpr int MUSIC_CODE_MUL = 4;
	static constexpr int MUSIC_CODE_DIV = 5;
	static constexpr int MUSIC_CODE_MODULO = 6;
	static constexpr int MUSIC_CODE_AND = 7;
	static constexpr int MUSIC_CODE_OR = 8;
	static constexpr int MUSIC_CODE_LEFT_SHIFT = 9;
	static constexpr int MUSIC_CODE_RIGHT_SHIFT = 10;
	static constexpr int MUSIC_CODE_END = -1;

	int code_length_ = 0;
	int code_[MUSIC_MAX_CODE_SIZE];
	int stack_[MUSIC_MAX_STACK_SIZE];
};

class Music
{
	Music(void);
	virtual ~Music(void);

private:
	static constexpr int MUSIC_NUM_PLAY_BLOCKS = 8;
	static constexpr int MUSIC_BUFFER_SIZE = (2048 + 512);
	static constexpr int MUSIC_RATE = 44100;
	static constexpr int MUSIC_NUM_CHANNELS = 1;
	static WAVEFORMATEX WFX;

	void FillNextBlock(void);

	void NextPlayBlock(void) {
		next_play_block_++;
		if (next_play_block_ >= MUSIC_NUM_PLAY_BLOCKS) next_play_block_ = 0;
	}

	int time = 0;

	// Windows playback code
	HWAVEOUT wave_out_;  // audio device handle
	WAVEHDR wave_header_[MUSIC_NUM_PLAY_BLOCKS];  // header of the audio block
	int next_play_block_ = 0;
	short audio_block_[MUSIC_NUM_PLAY_BLOCKS][MUSIC_BUFFER_SIZE * MUSIC_NUM_CHANNELS]; // The audio blocks

	friend DWORD WINAPI MusicThreadFunc(LPVOID lpParameter);
};

