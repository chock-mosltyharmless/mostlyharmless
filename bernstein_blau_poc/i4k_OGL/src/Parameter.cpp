#include "Parameter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool CopyToClipboard(char *strText)
{
	bool bRet = true;

	if (strlen(strText) > 0)
	{
		HGLOBAL		hGlobalMemory;
		LPVOID		lpGlobalMemory;

		// allocate global memory
		hGlobalMemory = GlobalAlloc(GHND, strlen(strText) + 1);
		if (hGlobalMemory)
		{
			// lock block to get a pointer to it
			lpGlobalMemory = GlobalLock(hGlobalMemory);
			// copy string to the block
			lpGlobalMemory = strcpy((char *)lpGlobalMemory, strText);
			// free memory
			GlobalUnlock(hGlobalMemory);

			// open clipboard for task
			if (OpenClipboard(0))
			{
				EmptyClipboard();
				// try writing if successful
				if (!SetClipboardData(CF_TEXT, hGlobalMemory))
				{
					// error writing to clipboard
					bRet = false;
				}
				CloseClipboard();
			}
		}
	}
	return bRet;
}

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
	static char string[NUM_PARAMETERS*12];
	char tmpString[12];
	
	/* Stop midi */
	for (unsigned int devID = 0; devID < numDevices; devID++)
	{
		MMRESULT rc;
		rc = midiInReset(midiInDevice[devID]);
		rc = midiInStop(midiInDevice[devID]);
		rc = midiInClose(midiInDevice[devID]);
	}

	/* Save to clipboard... */
	string[0] = 0;
	for (int par = 0; par < NUM_PARAMETERS; par++)
	{
		if (changed[par])
		{
			sprintf(tmpString, "%d:%.2f(%d) ", par, value[par], (int)(value[par]*128.0f + 0.49f));
			strcat(string, tmpString);
		}
	}

	CopyToClipboard(string);
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
		int first = (dwParam1 >> 8) & 255;
		int second = (dwParam1 >> 16) & 255;
		params.changed[first] = true;
		params.value[first] = (float)second / 128.0f;
	}
}

Parameter params;
