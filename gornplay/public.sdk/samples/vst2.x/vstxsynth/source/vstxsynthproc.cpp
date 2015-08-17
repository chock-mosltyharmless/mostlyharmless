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
#include <stdio.h>
#include <Windows.h>
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

#ifdef SAVE_MUSIC
int VstXSynth::firstNoteTime = -1;
#endif

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
		fPhase[i] = 0.f;
	}
	fScaler = (float)((double)PI / 44100.);	// we don't know the sample rate yet
	VstInt32 i;
	currentNote = nextNote = 0;
	currentVelocity = nextVelocity = 0;
	sampleID = 0;
	iADSR = 0;
	adsrVolume = 0.0f;
	adsrQuak = 0.0f;
	adsrDistort = 0.0f;
	adsrNoise = 0.0f;

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

	// Set null shapes
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			fShape[j][i] = 0.0f;
		}
	}

	// Clear reverberation buffer
	for (int i = 0; i < NUM_STEREO_VOICES; i++)
	{
		for (int j = 0; j < MAX_DELAY_LENGTH; j++)
		{
			reverbBuffer[i][j] = 0;
		}
		reverbBufferLength[i] = 1;
	}

#ifdef SAVE_MUSIC
	savedNoteID = 0;
	for (int i = 0; i < kNumParams; i++)
	{
		savedInstrumentID = 0;
	}
#endif
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	float baseFreq = freqtab[currentNote & 0x7f] * fScaler;
	//float vol = (float)(curProgram->fVolume * (double)currentVelocity * midiScaler) * 4.0f;
	float vol = (float)((double)currentVelocity * midiScaler);

	// Some intermediate setting of shape stuff
	for (int i = 0; i < NUM_OVERTONES; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			fShape[j][i] = expRandomBuffer[i + curProgram->iShape[j]*NUM_OVERTONES];
		}
	}

	// loop
	while (--sampleFrames >= 0)
	{
		// Check if a new note has to be played
		if (deathCounter == 0)
		{
			noteOn();
			baseFreq = freqtab[currentNote & 0x7f] * fScaler;
			//vol = (float)(curProgram->fVolume * (double)currentVelocity * midiScaler) * 4.0f;
			vol = (float)((double)currentVelocity * midiScaler);
		}
		if (deathCounter > 0) deathCounter--;

		// Process ADSR envelope
#if 0
		switch (iADSR)
		{
		case 0: // Attack
			fADSRVal += fAttack / 256.0f;
			if (fADSRVal > 1.0f)
			{
				iADSR = 1;
				fADSRVal = 1.0f;
			}
			break;
		case 1: // Decay
			fADSRVal -= fSustain;
			fADSRVal *= (1.0f - fDecay / 1024.0f);
			fADSRVal += fSustain;
			break;
		case 2: // Release
			fADSRVal *= (1.0f - fRelease / 1024.0f);
			break;
		default:
			break;
		}
		if (fADSRVal < 1.0f / 65536.0f) fADSRVal = 0.0f;
#else
		switch (iADSR)
		{
		case 0: // Attack
			fADSRVal += (1.0f - fADSRVal) * (curProgram->fADSRSpeed[0] / 1024.0f);
			break;
		default: // Not needed at all
			break;
		}
		// Go from attack to decay
		if (iADSR == 0 && fADSRVal > 0.75) iADSR = 1;
#endif

		// interpolate volume according to ADSR envelope
		adsrVolume += (curProgram->fVolume[iADSR + 1] - adsrVolume) *
					  (curProgram->fADSRSpeed[iADSR] / 1024.0f);
		adsrQuak += (curProgram->fQuak[iADSR + 1] - adsrQuak) *
					(curProgram->fADSRSpeed[iADSR] / 1024.0f);
		adsrDistort += (curProgram->fDistort[iADSR + 1] - adsrDistort) *
					   (curProgram->fADSRSpeed[iADSR] / 1024.0f);
		adsrNoise += (curProgram->fNoise[iADSR + 1] - adsrNoise) *
					 (curProgram->fADSRSpeed[iADSR] / 1024.0f);
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			adsrShape[i] += (fShape[iADSR + 1][i] - adsrShape[i]) *
						    (curProgram->fADSRSpeed[iADSR] / 1042.0f);
		}

		// deathcounter volume
		float deathVolume = 1.0f;
		if (deathCounter >= 0 && deathCounter < DEATH_COUNTER_SAMPLES) 
		{
			deathVolume = deathCounter * 1.0f / (float)DEATH_COUNTER_SAMPLES;
		}

		float outAmplitude[NUM_STEREO_VOICES] = {0.0f};

		int maxOvertones = (int)(3.0f / baseFreq);
		float overtoneLoudness = 1.0f;
		float overallLoudness = 0.0f;
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			if (i < maxOvertones)
			{
				outAmplitude[i % NUM_STEREO_VOICES] += (float)sin(fPhase[i]) * overtoneLoudness *
					adsrShape[i];
				fPhase[i] += baseFreq * (i+1);
				while (fPhase[i] > 2.0f * PI) fPhase[i] -= 2.0f * (float)PI;
			}
			overallLoudness += overtoneLoudness * adsrShape[i];
			overtoneLoudness *= adsrQuak * 2.0f;
		}

		// Ring modulation with noise
		for (int i = 0; i < NUM_STEREO_VOICES; i++)
		{
			// Ring modulation with noise
			outAmplitude[i] *= 1.0f + (lowNoise[sampleID % RANDOM_BUFFER_SIZE] - 1.0f) * adsrNoise;				
		}

		// adjust amplitude
		for (int i = 0; i < NUM_STEREO_VOICES; i++)
		{
			outAmplitude[i] /= overallLoudness;

			// Distort
			float distortMult = exp(6.0f*adsrDistort);
			outAmplitude[i] *= distortMult;
			outAmplitude[i] = 2.0f * (1.0f / (1.0f + exp(-outAmplitude[i])) - 0.5f);
		}

#if 0
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
					outAmplitude[j] += (float)sin(fPhase[i][j] + fStereo * (2 * PI * randomBuffer[i*NUM_Stereo_VOICES + j])) *
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

#endif

		// Put everything into the reverb buffers
		int reverbPos = sampleID % MAX_DELAY_LENGTH;
		for (int j = 0; j < NUM_STEREO_VOICES; j++)
		{
			reverbBuffer[reverbPos][j] = outAmplitude[j] * vol * adsrVolume * deathVolume;
			
			// Do the reverb feedback
			int fromBuffer = (j + 1) % NUM_STEREO_VOICES;
			int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[fromBuffer]);
			reverbBuffer[reverbPos][j] += 0.5f * curProgram->fDelayFeed * reverbBuffer[fromLocation % MAX_DELAY_LENGTH][fromBuffer];
			reverbBuffer[reverbPos][j] += 0.5f * curProgram->fDelayFeed * reverbBuffer[(fromLocation+1) % MAX_DELAY_LENGTH][(fromBuffer + 1) % NUM_STEREO_VOICES];
			if (fabsf(reverbBuffer[reverbPos][j]) < 1.0e-12) reverbBuffer[reverbPos][j] = 0.0f;
		}

		*out1 = 0;
		*out2 = 0;
		for (int j = 0; j < NUM_STEREO_VOICES; j += 2)
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

		out1++;
		out2++;

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
#if 0
			if (!velocity && (note == currentNote))
				noteOff ();
			else
#endif
			{
				if (!velocity && (note == currentNote))
				{
					noteOff();
				}
				else
				{
					nextNote = note;
					nextVelocity = velocity;

					deathCounter = event->deltaFrames + DEATH_COUNTER_SAMPLES;
					if (event->deltaFrames < 0)
					{
						deathCounter = DEATH_COUNTER_SAMPLES;
					}
				}
			}
		}
		else if (status == 0xb0)
		{
			if (midiData[1] == 0x7e || midiData[1] == 0x7b)	// all notes off
			{
				noteOff();
			}
		}
		event++;
	}
	return 1;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOn ()
{
#ifdef SAVE_MUSIC
	if (firstNoteTime < 0 && velocity > 0) firstNoteTime = sampleID;

	if (firstNoteTime >= 0)
	{
		// Overwrite last note if next one comes shortly after...
		if (savedNoteID > 0 && sampleID - savedNoteTime[savedNoteID-1] < 1024) savedNoteID--;
		savedNoteTime[savedNoteID] = sampleID;
		savedNote[savedNoteID] = note;
		savedVelocity[savedNoteID] = velocity;
		savedNoteID++;
	}
#endif

	deathCounter = -1; // no more death counting
	currentNote = nextNote;
	currentVelocity = nextVelocity;
	
	// Initialize ADSR and all interpolators
	iADSR = 0;
	fADSRVal = 0.0f;
	adsrVolume = curProgram->fVolume[0];
	adsrQuak = curProgram->fQuak[0];
	adsrDistort = curProgram->fDistort[0];
	adsrNoise = curProgram->fNoise[0];

	// This is just for debugging
	nextNote = 0;
	nextVelocity = 0;

	// reset instrument
	for (int i = 0; i < NUM_OVERTONES; i++)
	{
		fPhase[i] = 0.0f;
	}

	// Set the reverberation buffer length at key start (no interpolation)
	reverbBufferLength[0] = curProgram->iDelayLength * DELAY_MULTIPLICATOR + 1;
	reverbBufferLength[1] = curProgram->iDelayLength * DELAY_MULTIPLICATOR * 7 / 17 + 1;
	reverbBufferLength[2] = curProgram->iDelayLength * DELAY_MULTIPLICATOR * 13 / 23 + 1;
	reverbBufferLength[3] = curProgram->iDelayLength * DELAY_MULTIPLICATOR * 11 / 13 + 1;

	// Set the sound shape at the beginning
	for (int i = 0; i < NUM_OVERTONES; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			fShape[j][i] = expRandomBuffer[i + curProgram->iShape[j]*NUM_OVERTONES];
		}
		adsrShape[i] = fShape[0][i];
	}

#if 0
	midiDelaySamples = -1;
	if (note != -1)
	{
		currentNote = note;
		currentVelocity = velocity;
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
	else
	{
		iADSR = 2;
	}
#endif

	// Write everything out, we can overwrite at the next note.
#ifdef SAVE_MUSIC
	FILE *fid;
	char filename[1024];
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
#if 0
	sprintf_s(filename, 1024, "C:/vierKA/music.%d.txt",
		sysTime.wYear, sysTime.wMonth,
		sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, (int)this);
#else
	sprintf_s(filename, 1024, "C:/vierKA/music.%d.txt", (int)this);
#endif
	fopen_s(&fid, filename, "wb");

	// write Instrument information
	// This assumes that no automation is active anymore...
	fprintf(fid, "#ifndef F_VOLUME\n");
	fprintf(fid, "#define F_VOLUME               0\n");
	fprintf(fid, "#define K_DURATION             1\n");
	fprintf(fid, "#define K_ATTACK               2\n");
	fprintf(fid, "#define K_DECAY                3\n");
	fprintf(fid, "#define K_SUSTAIN              4\n");
	fprintf(fid, "#define K_RELEASE              5\n");
	fprintf(fid, "#define K_QUAKINESS_START      6\n");
	fprintf(fid, "#define K_QUAKINESS_END        7\n");
	fprintf(fid, "#define K_SOUND_SHAPE_START    8\n");
	fprintf(fid, "#define K_SOUND_SHAPE_END      9\n");
	fprintf(fid, "#define K_MODULATION_AMOUNT   10\n");
	fprintf(fid, "#define K_MODULATION_SPEED    11\n");
	fprintf(fid, "#define K_DETUNE              12\n");
	fprintf(fid, "#define K_STEREO              13\n");
	fprintf(fid, "#define K_NOISE_START         14\n");
	fprintf(fid, "#define K_NOISE_END           15\n");
	fprintf(fid, "#define K_DELAY_FEED          16\n");
	fprintf(fid, "#define K_DELAY_LENGTH        17\n");
	fprintf(fid, "#endif\n\n");

#if 1
	// Print single instrument parameters
	fprintf(fid, "#pragma data_seg(\".instrumentParams\")\n");
	fprintf(fid, "unsigned char instrumentParams_%d[] = {\n", (int)this);
	fprintf(fid, "  %d,", (int)(fVolume*127));
	fprintf(fid, "  %d,", (int)(fDuration*127));
	fprintf(fid, "  %d,", (int)(fAttack*127));
	fprintf(fid, "  %d,", (int)(fDecay*127));
	fprintf(fid, "  %d,", (int)(fSustain*127));
	fprintf(fid, "  %d,", (int)(fRelease*127));
	fprintf(fid, "  %d,", (int)(fQuakinessStart*127));
	fprintf(fid, "  %d,", (int)(fQuakinessEnd*127));
	fprintf(fid, "  %d,", iSoundShapeStart);
	fprintf(fid, "  %d,", iSoundShapeEnd);
	fprintf(fid, "  %d,", (int)(fModulationAmount*127));
	fprintf(fid, "  %d,", (int)(fModulationSpeed*127));
	fprintf(fid, "  %d,", (int)(fDetune*127));
	fprintf(fid, "  %d,", (int)(fStereo*127));
	fprintf(fid, "  %d,", (int)(fNoiseStart*127));
	fprintf(fid, "  %d,", (int)(fNoiseEnd*127));
	fprintf(fid, "  %d,", (int)(fDelayFeed*127));
	fprintf(fid, "  %d\n", iDelayLength);
	fprintf(fid, "};\n\n");
#else // print timed instrument data
	fprintf(fid, "#define NUM_INSTRUMENT_DATA_%d %d\n\n", (int)this, savedInstrumentID);
	fprintf(fid, "#pragma data_seg(\".savedInstrumentTime\")\n");
	fprintf(fid, "unsigned char savedInstrumetTime_%d[] = {\n", (int)this);
	int lastInstTime = firstNoteTime;
	for (int i = 0; i < savedInstrumentID; i++)
	{
		fprintf(fid, " %d,", (savedInstrumentTime[i] - lastInstTime) / 4134);
		if (i % 32 == 31) fprintf(fid, "\n ");
		lastInstTime = savedInstrumentTime[i];
	}
	fprintf(fid, "};\n\n");

	fprintf(fid, "#pragma data_seg(\".savedInstrumentParam\")\n");
	fprintf(fid, "signed char savedInstrumentParam_%d[%d][%d] = {\n ", (int)this, kNumParams, savedInstrumentID);
	for (int idx = 0; idx < kNumParams; idx++)
	{
		fprintf(fid, " { ");
		int lastParam = 0;
		for (int i = 0; i < savedInstrumentID; i++)
		{
			fprintf(fid, " %d,", savedInstrumentParameter[idx][i] - lastParam);
			if (i % 32 == 31) fprintf(fid, "\n   ");
			//lastParam = savedInstrumentParameter[idx][i];
		}
		fprintf(fid, "},\n");
	}
	fprintf(fid, "};\n\n");
#endif

	// write the note stuff
	fprintf(fid, "#define NUM_NOTES_%d %d\n\n", (int)this, savedNoteID);
	fprintf(fid, "#pragma data_seg(\".savedNoteTime\")\n");
	fprintf(fid, "unsigned char savedNoteTime_%d[] = {\n ", (int)this);
	int lastTime = firstNoteTime;
	for (int i = 0; i < savedNoteID; i++)
	{
		fprintf(fid, " %d,", (savedNoteTime[i] - lastTime) / 4134);
		if (i % 32 == 31) fprintf(fid, "\n ");
		lastTime = savedNoteTime[i];
	}
	// There is one final 255 to wait it out...
	fprintf(fid, " 255};\n\n");

	fprintf(fid, "#pragma data_seg(\".savedNote\")\n");
	fprintf(fid, "signed char savedNote_%d[] = {\n ", (int)this);
	int lastNote = 0;
	for (int i = 0; i < savedNoteID; i++)
	{
		if (savedNote[i] >= 0)
		{
			// note on, save delta
			fprintf(fid, " %d,", savedNote[i] - lastNote);
			lastNote = savedNote[i];
		}
		else
		{
			// note off, save -128
			fprintf(fid, " -128,");
		}
		if (i % 32 == 31) fprintf(fid, "\n ");		
	}
	fprintf(fid, "};\n\n");

#if 0
	fprintf(fid, "#pragma data_seg(\".savedVelocity\")\n");
	fprintf(fid, "signed char savedVelocity[] = {\n ");
	int lastVelocity = 0;
	for (int i = 0; i < savedNoteID; i++)
	{
		if (savedVelocity[i] >= 0)
		{
			// note on
			fprintf(fid, " %d,", savedVelocity[i] - lastVelocity);
			lastVelocity = savedVelocity[i];
		}
		else
		{
			// note off
			fprintf(fid, " 0,");
		}
		if (i % 32 == 31) fprintf(fid, "\n");
	}
	fprintf(fid, "};\n\n");
#endif

	fclose(fid);
#endif
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
