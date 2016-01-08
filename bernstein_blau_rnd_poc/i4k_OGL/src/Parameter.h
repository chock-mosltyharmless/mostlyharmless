#pragma once

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <Mmsystem.h>

#define NUM_PARAMETERS 128
#define MAX_NUM_DEVICES 32
#define MIDI_DATA_SIZE 1024

class Parameter
{
public:
	Parameter(void);
	~Parameter(void);

public:
	/* In the range 0.0 to 1.0 */
	float getParam(int index, float defaultValue = 0.0f);
	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

private:
	float value[NUM_PARAMETERS];
	bool changed[NUM_PARAMETERS];
	
	/* Midi device stuff */
	UINT numDevices;
	HMIDIIN midiInDevice[MAX_NUM_DEVICES];
	MIDIHDR midiInHeader[MAX_NUM_DEVICES];
	unsigned char midiData[MAX_NUM_DEVICES][MIDI_DATA_SIZE];
};

extern Parameter params;