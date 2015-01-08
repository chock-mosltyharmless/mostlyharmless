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
#define SAMPLE_TICK_DURATION (1.0f / 32768.0f)
#define REMAIN_DC_FALLOFF 0.99f

// Midi-relevant constants
const double midiScaler = (1. / 127.);
static float freqtab[kNumFrequencies];

// Moog x-pass from musicdsp.org:
#if 0
	// Moog 24 dB/oct resonant lowpass VCF
	// References: CSound source code, Stilson/Smith CCRMA paper.
	// Modified by paul.kellett@maxim.abel.co.uk July 2000

	  float f, p, q;             //filter coefficients
	  float b0, b1, b2, b3, b4;  //filter buffers (beware denormals!)
	  float t1, t2;              //temporary buffers

	// Set coefficients given frequency & resonance [0.0...1.0]

	  q = 1.0f - frequency;
	  p = frequency + 0.8f * frequency * q;
	  f = p + p - 1.0f;
	  q = resonance * (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));

	// Filter (in [-1.0...+1.0])

	  in -= q * b4;                          //feedback
	  t1 = b1;  b1 = (in + b0) * p - b1 * f;
	  t2 = b2;  b2 = (b1 + t1) * p - b2 * f;
	  t1 = b3;  b3 = (b2 + t2) * p - b3 * f;
				b4 = (b3 + t1) * p - b4 * f;
	  b4 = b4 - b4 * b4 * b4 * 0.166667f;    //clipping
	  b0 = in;

	// Lowpass  output:  b4
	// Highpass output:  in - b4;
	// Bandpass output:  3.0f * (b3 - b4);
#endif

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------

float VstXSynth::moogFilter(float frequency, float resonance, float in)
{
	float t1, t2;
	float q = 1.0f - frequency;
	float p = frequency + 0.8f * frequency * q;
	float f = p + p - 1.0f;
	q = resonance * (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));

	// I should check this... Maybe this is not so cool?
	if (in > 1.414f) in = 1.414f;
	if (in < -1.414f) in = -1.414f;
	in = in - in * in * in * 0.166667f;    //clipping

	in -= q * b4; // feedback
	t1 = b1;  b1 = (in + b0) * p - b1 * f;
	t2 = b2;  b2 = (b1 + t1) * p - b2 * f;
	t1 = b3;  b3 = (b2 + t2) * p - b3 * f;
	b4 = (b3 + t1) * p - b4 * f;
	
	b0 = in;

	// Check for denormals
#if 0
	if (fabsf(b0) < 1.0f / 1024.0f / 1024.0f) b0 = 0.0f;
	if (fabsf(b1) < 1.0f / 1024.0f / 1024.0f) b1 = 0.0f;
	if (fabsf(b2) < 1.0f / 1024.0f / 1024.0f) b2 = 0.0f;
	if (fabsf(b3) < 1.0f / 1024.0f / 1024.0f) b3 = 0.0f;
	if (fabsf(b4) < 1.0f / 1024.0f / 1024.0f) b4 = 0.0f;
#endif

	return b4; // low-pass
}

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
	for (int i = 0; i < NUM_OVERTONES; i++)
	{
		for (int j = 0; j < NUM_Stereo_VOICES; j++)
		{
			fPhase[i][j] = 0.f;
		}
	}
	fModulationPhase = 0.f;
	iADSR = 0;
	fADSRVal = 0.0f;
	fScaler = (float)((double)PI / 44100.);	// we don't know the sample rate yet
	currentVelocity = 0;
	currentDelta = currentNote = currentDelta = 0;
	VstInt32 i;
	noteOn (0, 0, 0);
	sampleID = 0;

	// Initialize moog filter parameters
	b0 = b1 = b2 = b3 = b4 = 0.0f;

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
		lowNoise[i] = 16.0f * (randomBuffer[i] - 0.5f);
	}

	// Ring-low-pass-filtering of lowPass
	// Use a one-pole
	float oldVal = 0.0f;
	for (int i = 0; i < RANDOM_BUFFER_SIZE * 8; i++)
	{
		lowNoise[i % RANDOM_BUFFER_SIZE] = 1.0f / 8.0f * lowNoise[i % RANDOM_BUFFER_SIZE] + (1.0f - 1.0f / 8.0f) * oldVal;
		oldVal = lowNoise[i % RANDOM_BUFFER_SIZE];
	}

	// Clear reverberation buffer
	for (int i = 0; i < NUM_Stereo_VOICES; i++)
	{
		for (int j = 0; j < MAX_DELAY_LENGTH; j++)
		{
			reverbBuffer[i][j] = 0;
		}
		reverbBufferLength[i] = 1;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	float baseFreq = freqtab[currentNote & 0x7f] * fScaler;
	float vol = (float)(fVolume * (double)currentVelocity * midiScaler) * 4.0f;
		
	if (currentDelta > 0) // PROBLEM: THERE IS NO REVERB!
	{
		if (currentDelta >= sampleFrames)	// future
		{
			currentDelta -= sampleFrames;
			for (int i = 0; i < sampleFrames; i++)
			{
				for (int j = 0; j < NUM_Stereo_VOICES; j++)
				{
					fRemainDC[j] *= REMAIN_DC_FALLOFF;
					fLastOutput[j] = fRemainDC[j];
				}
				*out1 = fRemainDC[0] + fRemainDC[2];
				*out2 = fRemainDC[1] + fRemainDC[3];
				out1++;
				out2++;
			}
			return;
		}

		for (int i = 0; i < currentDelta; i++)
		{
			for (int j = 0; j < NUM_Stereo_VOICES; j++)
			{
				fRemainDC[j] *= REMAIN_DC_FALLOFF;
				fLastOutput[j] = fRemainDC[j];
			}
			*out1 = fRemainDC[0] + fRemainDC[2];
			*out2 = fRemainDC[1] + fRemainDC[3];
			out1++;
			out2++;
		}
		sampleFrames -= currentDelta;
		currentDelta = 0;
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
		float modT = fModulationPhase - (float)(int)(fModulationPhase);

		float outAmplitude[NUM_Stereo_VOICES] = {0};

		int maxOvertones = (int)(3.0f / baseFreq);
		float overtoneLoudness = 1.0f;
		float overallLoudness = 0.0f;
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			float soundShape = 1.0f;
			if (i != 0 || true)
			{
				float soundShapeStart = expRandomBuffer[i + iSoundShapeStart*NUM_OVERTONES];
				float soundShapeEnd = expRandomBuffer[i + iSoundShapeEnd*NUM_OVERTONES];
				soundShape = relTimePoint * soundShapeEnd + (1.0f - relTimePoint) * soundShapeStart;
			}

			if (i < maxOvertones)
			{
				// Modulation:
				float startMod = expRandomBuffer[i + (int)(fModulationPhase)*NUM_OVERTONES];
				float endMod = expRandomBuffer[i + (int)(fModulationPhase)*NUM_OVERTONES + NUM_OVERTONES];
				float modulation = modT * endMod + (1.0f - modT) * startMod;
				modulation = .5f + (modulation - 0.5f) * fModulationAmount;

				for (int j = 0; j < NUM_Stereo_VOICES; j++)
				{
					outAmplitude[j] += sin(fPhase[i][j] + fStereo * (2 * PI * randomBuffer[i*NUM_Stereo_VOICES + j])) *
						overtoneLoudness * soundShape * modulation;

					fPhase[i][j] += baseFreq * (i+1) * (1.0f + fStereo/64.0f * (randomBuffer[i] - 0.5f)) *
					//fPhase[i][j] += baseFreq * (i+1) *
						 (1.0f + fDetune * (randomBuffer[i + iSoundShapeEnd*NUM_OVERTONES] - 0.5f));
					while (fPhase[i][j] > 2.0f * PI) fPhase[i][j] -= 2.0f * (float)PI;
				}
			}
			overallLoudness += overtoneLoudness * soundShape;
			float quakiness = relTimePoint * fQuakinessEnd + (1.0f - relTimePoint) * fQuakinessStart;
			overtoneLoudness *= quakiness * 2.0f;
		}
		
		float noiseAmount = relTimePoint * fNoiseEnd + (1.0f - relTimePoint) * fNoiseStart;
		for (int j = 0; j < NUM_Stereo_VOICES; j++)
		{
			// Adjust volume
			outAmplitude[j] /= overallLoudness;

			// Ring modulation with noise
			outAmplitude[j] *= 1.0f + (lowNoise[sampleID % RANDOM_BUFFER_SIZE] - 1.0f) * noiseAmount;
		}

		// Put everything into the reverb buffers
		int reverbPos = sampleID % MAX_DELAY_LENGTH;
		for (int j = 0; j < NUM_Stereo_VOICES; j++)
		{
			reverbBuffer[reverbPos][j] = outAmplitude[j] * vol * fADSRVal + fRemainDC[j];
			fRemainDC[j] *= REMAIN_DC_FALLOFF;
			fLastOutput[j] = reverbBuffer[reverbPos][j];
			
			// Do the reverb feedback
			int fromBuffer = (j + 1) % NUM_Stereo_VOICES;
			int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[fromBuffer]) % 
				MAX_DELAY_LENGTH;
			reverbBuffer[reverbPos][j] += fDelayFeed * reverbBuffer[fromLocation][fromBuffer];
		}

		*out1 = 0;
		*out2 = 0;
		for (int j = 0; j < NUM_Stereo_VOICES; j += 2)
		{
			//*out1 += outAmplitude[j];
			//*out2 += outAmplitude[j + 1];
			*out1 += reverbBuffer[reverbPos][j];
			*out2 += reverbBuffer[reverbPos][j+1];
		}

		// Apply moog filter
		//float filterFreq = (float)exp(fFilterStart * 6.0f - 6.0f);
		//*out1 = moogFilter(filterFreq, fResoStart, *out1);
		//*out2 = moogFilter(filterFreq, fResoStart, *out2);

		*out1++;
		*out2++;
		fModulationPhase += fModulationSpeed / 256.0f;
		fTimePoint += SAMPLE_TICK_DURATION;
		if (fTimePoint > fDuration) fTimePoint = fDuration;
		while (fModulationPhase > RANDOM_BUFFER_SIZE/2/NUM_OVERTONES) fModulationPhase -= RANDOM_BUFFER_SIZE/2/NUM_OVERTONES;

		sampleID++;
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
	for (int i = 0; i < NUM_OVERTONES; i++)
	{
		for (int j = 0; j < NUM_Stereo_VOICES; j++)
		{
			fPhase[i][j] = 0;
		}
	}
	iADSR = 0;
	fADSRVal = 0.0f;
	fTimePoint = 0.0f;

	for (int i = 0; i < NUM_Stereo_VOICES; i++)
	{
		fRemainDC[i] = fLastOutput[i];
	}

	// Set the reverberation buffer length at key start (no interpolation)
	reverbBufferLength[0] = iDelayLength * DELAY_MULTIPLICATOR + 1;
	reverbBufferLength[1] = iDelayLength * DELAY_MULTIPLICATOR * 7 / 17 + 1;
	reverbBufferLength[2] = iDelayLength * DELAY_MULTIPLICATOR * 13 / 23 + 1;
	reverbBufferLength[3] = iDelayLength * DELAY_MULTIPLICATOR * 11 / 13 + 1;
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
