#include "stdafx.h"
#include "Parameter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void registerParameterChange(int keyID);

Parameter::Parameter(void)
{
	/* Initialize values */
	for (int i = 0; i < NUM_PARAMETERS; i++)
	{
		value[i] = 0.0f;
		changed[i] = false;
	}

	/* Open Midi device */
	numDevices = midiInGetNumDevs();
	if (numDevices > MAX_NUM_DEVICES) numDevices = MAX_NUM_DEVICES;
	for (unsigned int devID = 0; devID < numDevices; devID++)
	{
		MMRESULT rc;
		rc = midiInOpen(&(midiInDevice[devID]), devID, (DWORD_PTR)MidiInProc, NULL, CALLBACK_FUNCTION);
		/* Create input buffers */
		midiInHeader[devID].lpData = (LPSTR)midiData[devID];
		midiInHeader[devID].dwBufferLength = MIDI_DATA_SIZE;
		midiInHeader[devID].dwFlags = 0;
		rc = midiInPrepareHeader(midiInDevice[devID], &midiInHeader[devID], sizeof(midiInHeader[devID]));
		rc = midiInAddBuffer(midiInDevice[devID], &midiInHeader[devID], sizeof(midiInHeader[devID]));
		rc = midiInStart(midiInDevice[devID]);
	}
}

Parameter::~Parameter(void)
{
	/* Stop midi */
	for (unsigned int devID = 0; devID < numDevices; devID++)
	{
		MMRESULT rc;
		rc = midiInReset(midiInDevice[devID]);
		rc = midiInStop(midiInDevice[devID]);
		rc = midiInClose(midiInDevice[devID]);
	}
}

float Parameter::getParam(int index, float defaultValue)
{
	if (!changed[index])
	{
		value[index] = defaultValue;
	}
	return value[index];
}

void CALLBACK Parameter::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (wMsg == MIM_DATA)
	{
		// Korg Nano Control
		int first = (dwParam1 >> 8) & 255;
		int second = (dwParam1 >> 16) & 255;
		params.changed[first] = true;
		params.value[first] = (float)second / 100.0f;

		// escalate the parameter change to the intro
		registerParameterChange(first);
	}
}

Parameter params;
