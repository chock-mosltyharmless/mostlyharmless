//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:27 $
//
// Category     : VST 2.x SDK Samples
// Filename     : vstxsynthproc.cpp
// Created by   : Steinberg Media Technologies
// Description  : Example VstXSynth
//
// A simple 2 oscillators test 'synth',
// Each oscillator has waveform, frequency, and volume
//
// *very* basic monophonic 'synth' example. you should not attempt to use this
// example 'algorithm' to start a serious virtual instrument; it is intended to demonstrate
// how VstEvents ('MIDI') are handled, but not how a virtual analog synth works.
// there are numerous much better examples on the web which show how to deal with
// bandlimited waveforms etc.
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include <math.h>
#include "vstxsynth.h"

enum
{
	kNumFrequencies = 128,	// 128 midi notes
};

// Constants for sound stuff
#define NUM_OVERTONES 12
#define SAMPLE_TICK_DURATION (1.0f / 32768.0f)
#define REMAIN_DC_FALLOFF 0.99f

// Midi-relevant constants
const double midiScaler = (1. / 127.);
static float freqtab[kNumFrequencies];

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
void VstXSynth::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);
	fScaler = (float)((double)2*PI / (double)sampleRate);
}

// Returns a value in the range [0;1[
float VstXSynth::frand(void)
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	//return (seed >> 8) % 65535;
	return (float)((seed>>8)&65535) * (1.0f/65536.0f);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setBlockSize (VstInt32 blockSize)
{
	AudioEffectX::setBlockSize (blockSize);
	// you may need to have to do something here...
}

//-----------------------------------------------------------------------------------------
void VstXSynth::initProcess ()
{
	fPhase = 0.f;
	iADSR = 0;
	fADSRVal = 0.0f;
	fScaler = (float)((double)PI / 44100.);	// we don't know the sample rate yet
	currentVelocity = 0;
	currentDelta = currentNote = currentDelta = 0;
	VstInt32 i;
	noteOn (0, 0, 0);

	// make frequency (Hz) table
	double k = 1.059463094359;	// 12th root of 2
	double a = 6.875;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (i = 0; i < kNumFrequencies; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}

	// Make table with random number
	seed = 1;
	for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
	{
		randomBuffer[i] = frand();
		expRandomBuffer[i] = (float)exp(4.0f * (randomBuffer[i] - 1));
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	float baseFreq = freqtab[currentNote & 0x7f] * fScaler;
	float vol = (float)(fVolume * (double)currentVelocity * midiScaler) * 4.0f;
		
	if (currentDelta > 0)
	{
		if (currentDelta >= sampleFrames)	// future
		{
			currentDelta -= sampleFrames;
			memset(out1, 0, sampleFrames * sizeof (float));
			memset(out2, 0, sampleFrames * sizeof (float));
			return;
		}
		memset (out1, 0, currentDelta * sizeof (float));
		memset (out2, 0, currentDelta * sizeof (float));
		out1 += currentDelta;
		out2 += currentDelta;
		sampleFrames -= currentDelta;
		currentDelta = 0;
		fLastOutput[0] = 0.0f;
		fLastOutput[1] = 0.0f;
	}

	// loop
	while (--sampleFrames >= 0)
	{
		// Process ADSR envelope
		switch (iADSR)
		{
		case 0:
			fADSRVal += fAttack / 512.0f;
			if (fADSRVal > 1.0f)
			{
				iADSR = 1;
				fADSRVal = 1.0f;
			}
			break;
		case 2:
			fADSRVal *= (1.0f - fRelease / 2048.0f);
			break;
		default:
			break;
		}

		// The relative time point from instrument start to instrument end
		float relTimePoint = fTimePoint / (fDuration + 1.0f / 256.0f);

		float outAmplitude = 0.0f;

		int maxOvertones = (int)(3.0f / baseFreq);
		float overtoneLoudness = 1.0f;
		float overallLoudness = 0.0f;
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			if (i < maxOvertones)
			{
				float soundShapeStart = randomBuffer[i + iSoundShapeStart*NUM_OVERTONES];
				float soundShapeEnd = randomBuffer[i + iSoundShapeEnd*NUM_OVERTONES];
				float soundShape = relTimePoint * soundShapeEnd + (1.0f - relTimePoint) * soundShapeStart;
				outAmplitude += sin(fPhase * (i+1)) * overtoneLoudness * soundShape;
			}
			overallLoudness += overtoneLoudness;
			float quakiness = relTimePoint * fQuakinessEnd + (1.0f - relTimePoint) * fQuakinessStart;
			overtoneLoudness *= quakiness * 2.0f;
		}
		outAmplitude /= overallLoudness;

		fRemainDC[0] *= REMAIN_DC_FALLOFF;
		fRemainDC[1] *= REMAIN_DC_FALLOFF;
		*out1 = outAmplitude * vol * fADSRVal + fRemainDC[0];
		*out2 = outAmplitude * vol * fADSRVal+ fRemainDC[1];
		fLastOutput[0] = *out1;
		fLastOutput[1] = *out2;
		*out1++;
		*out2++;
		fPhase += baseFreq;
		fTimePoint += SAMPLE_TICK_DURATION;
		if (fTimePoint > fDuration) fTimePoint = fDuration;
		while (fPhase > 2.0f * PI) fPhase -= 2.0f * (float)PI;
	}
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::processEvents (VstEvents* ev)
{
	for (VstInt32 i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;

		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		VstInt32 status = midiData[0] & 0xf0;	// ignoring channel
		if (status == 0x90 || status == 0x80)	// we only look at notes
		{
			VstInt32 note = midiData[1] & 0x7f;
			VstInt32 velocity = midiData[2] & 0x7f;
			if (status == 0x80)
				velocity = 0;	// note off by velocity 0
			if (!velocity && (note == currentNote))
				noteOff ();
			else
				noteOn (note, velocity, event->deltaFrames);
		}
		else if (status == 0xb0)
		{
			if (midiData[1] == 0x7e || midiData[1] == 0x7b)	// all notes off
				noteOff ();
		}
		event++;
	}
	return 1;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOn (VstInt32 note, VstInt32 velocity, VstInt32 delta)
{
	currentNote = note;
	currentVelocity = velocity;
	currentDelta = delta;
	fPhase = 0;
	iADSR = 0;
	fADSRVal = 0.0f;
	fTimePoint = 0.0f;

	fRemainDC[0] = fLastOutput[0];
	fRemainDC[1] = fLastOutput[1];
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOff ()
{
	//currentVelocity = 0;
	iADSR = 2;
	// This must be done when a new note is started only!
	//fRemainDC[0] = fLastOutput[0];
	//fRemainDC[1] = fLastOutput[1];
}
