//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#ifndef PI
#define PI 3.1415
#endif
#define LOG_2_E 1.44269f
#define fScaler ((float)((double)2*PI / (double)MZK_RATE))

#define NUM_INSTRUMENTS 3
#define NUM_INSTRUMENT_PARAMETERS 18
// Number of additive overtones
#define NUM_OVERTONES 16
#define NUM_Stereo_VOICES 4

// Music time update parameter
#define SAMPLE_TICK_DURATION (1.0f / 32768.0f)

// 128 midi notes
#define kNumFrequencies 128
static float freqtab[kNumFrequencies];

// The multiplier to get from midi int to float
#define MIDI_INT_TO_FLOAT (1.0f / 127.0f)

// Size of a buffer with random numbers
#define RANDOM_BUFFER_SIZE 65536
#define DELAY_MULTIPLICATOR 128
#define MAX_DELAY_LENGTH (DELAY_MULTIPLICATOR * 130) // Some safety for miscalculation stuff...

float randomBuffer[RANDOM_BUFFER_SIZE];
float lowNoise[RANDOM_BUFFER_SIZE];
// The same as random Buffer, but contains exp(4*(randomBuffer-1))
float expRandomBuffer[RANDOM_BUFFER_SIZE];
float reverbBuffer[NUM_INSTRUMENTS][MAX_DELAY_LENGTH][NUM_Stereo_VOICES];
int reverbBufferLength[NUM_INSTRUMENTS][NUM_Stereo_VOICES]; // Actual length taken for pull-out
// The note index that the instrument is on
int currentNoteIndex[NUM_INSTRUMENTS];
int currentNote[NUM_INSTRUMENTS];
float fPhase[NUM_INSTRUMENTS][NUM_OVERTONES][NUM_Stereo_VOICES]; // Phase of the instrument
float fTimePoint[NUM_INSTRUMENTS];
float fModulationPhase[NUM_INSTRUMENTS];

// Current instrument parameter (possibly interpolated?)
int intInstParameter[NUM_INSTRUMENTS][NUM_INSTRUMENT_PARAMETERS];
float floatInstParameter[NUM_INSTRUMENTS][NUM_INSTRUMENT_PARAMETERS];

// attack values and stuff
int iADSR[NUM_INSTRUMENTS];
float fADSRVal[NUM_INSTRUMENTS];

static unsigned long seed;

// create random value between -65534 and 65534?
#pragma code_seg(".rand")
int rand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	return (seed >> 8) % 65535;
}

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
#pragma code_seg(".frand")
float frand()
{
	return (float)(rand()) * (1.0f/65536.0f);
}

#pragma code_seg(".exp2jo")
static double exp2jo(double f)
{
	__asm fld f;
	__asm fld st;
	__asm frndint;
	__asm fsub st(1), st;
	__asm fxch;
	__asm f2xm1;
	__asm fld1;
	__asm fadd;
	__asm fscale;
	__asm fstp st(1);
	__asm fstp f;

	return f;
}

// This method transforms the music data so that it can be used to generate samples
#pragma code_seg(".initMzkData")
__inline void init_mzk_data()
{
	// Create data tables
	// make frequency (Hz) table
	double k = 1.059463094359;	// 12th root of 2
	double a = 6.875;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (int i = 0; i < kNumFrequencies; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}

	// Make table with random number
	seed = 1;
	for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
	{
		randomBuffer[i] = frand();
		expRandomBuffer[i] = (float)exp2jo(LOG_2_E * 4.0f * (randomBuffer[i] - 1));
		lowNoise[i] = 16.0f * (randomBuffer[i] - 0.5f);
	}

	// Set the oscillator phases to zero
	for (int inst = 0; inst < NUM_INSTRUMENTS; inst++)
	{
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			for (int j = 0; j < NUM_Stereo_VOICES; j++)
			{
				fPhase[inst][i][j] = 0.f;
			}
		}

		// Don't play instrument
		iADSR[inst] = 3;
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
	for (int inst = 0; inst < NUM_INSTRUMENTS; inst++)
	{
		for (int i = 0; i < NUM_Stereo_VOICES; i++)
		{
			for (int j = 0; j < MAX_DELAY_LENGTH; j++)
			{
				reverbBuffer[inst][i][j] = 0;
			}
			reverbBufferLength[inst][i] = 1;
		}
	}

	// Get floating point instrument parameters
	for (int inst = 0; inst < NUM_INSTRUMENTS; inst++)
	{
		for (int param = 0; param < NUM_INSTRUMENT_PARAMETERS; param++)
		{
			floatInstParameter[inst][param] = instrumentParams[inst][param] * MIDI_INT_TO_FLOAT;
		}
	}
}

#pragma code_seg(".mzkPlayBlock")
void mzkPlayBlock(short *blockBuffer)
{
	static int startSampleID = 0;
	int sampleID;

	// clear audio block
	for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
	{
		// Stereo! --> 2 values
		blockBuffer[sample] = 0;
	}

	for (int instrument = 0; instrument < NUM_INSTRUMENTS; instrument++)
	{
		// Go over all samples
		sampleID = startSampleID;

		// Check if we go to next note
		if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0)
		{
			// Key on (or off)
			iADSR[instrument] = 0; // attack
			fADSRVal[instrument] = 0.0f; // starting up
			fTimePoint[instrument] = 0.0f;

			// Set the reverberation buffer length at key start (no interpolation)
			reverbBufferLength[instrument][0] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR + 1;
			reverbBufferLength[instrument][1] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 7 / 17 + 1;
			reverbBufferLength[instrument][2] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 13 / 23 + 1;
			reverbBufferLength[instrument][3] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 11 / 13 + 1;

			// Apply delta-note
			currentNote[instrument] += savedNote[instrument][currentNoteIndex[instrument]];

			// Set the oscillator phases to zero
			for (int i = 0; i < NUM_OVERTONES; i++)
			{
				for (int j = 0; j < NUM_Stereo_VOICES; j++)
				{
					fPhase[instrument][i][j] = 0.f;
				}
			}

			// Go to next note location
			currentNoteIndex[instrument]++;
		}
		savedNoteTime[instrument][currentNoteIndex[instrument]]--;

		// Get audio frequency
		float baseFreq = freqtab[currentNote[instrument] & 0x7f] * fScaler;
		float fVolume = floatInstParameter[instrument][F_VOLUME];
		float vol = fVolume * 4.0f;

		for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++)
		{
			// Process ADSR envelope
			float fAttack = floatInstParameter[instrument][K_ATTACK];
			float fDecay = floatInstParameter[instrument][K_DECAY];
			float fSustain = floatInstParameter[instrument][K_SUSTAIN];
			float fRelease = floatInstParameter[instrument][K_RELEASE];
			switch (iADSR[instrument])
			{
			case 0: // Attack
				fADSRVal[instrument] += fAttack / 256.0f;
				if (fADSRVal[instrument] > 1.0f)
				{
					iADSR[instrument] = 1;
					fADSRVal[instrument] = 1.0f;
				}
				break;
			case 1: // Decay
				fADSRVal[instrument] -= fSustain;
				fADSRVal[instrument] *= (1.0f - fDecay / 1024.0f);
				fADSRVal[instrument] += fSustain;
				break;
			case 2: // Release
				fADSRVal[instrument] *= (1.0f - fRelease / 1024.0f);
				break;
			default:
				break;
			}

			// fade out on new instrument
			if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0 &&
				sample >= MZK_BLOCK_SIZE - 1024)
			{
				fADSRVal[instrument] -= 1.0f / 1024.0f;
			}

			// Some optimziation?
			if (fADSRVal[instrument] < 1.0f / 65536.0f) fADSRVal[instrument] = 0.0f;

			// The relative time point from instrument start to instrument end, modulation time
			float fDuration = floatInstParameter[instrument][K_DURATION];
			float relTimePoint = fTimePoint[instrument] / (fDuration + 1.0f / 512.0f);
			float modT = fModulationPhase[instrument] - (float)(int)(fModulationPhase[instrument]);
					
			float outAmplitude[NUM_Stereo_VOICES] = {0};

			int maxOvertones = (int)(3.0f / baseFreq);
			float overtoneLoudness = 1.0f;
			float overallLoudness = 0.0f;
			for (int i = 0; i < NUM_OVERTONES; i++)
			{
				float soundShape = 1.0f;
				int iSoundShapeStart = instrumentParams[instrument][K_SOUND_SHAPE_START];
				int iSoundShapeEnd = instrumentParams[instrument][K_SOUND_SHAPE_END];

				float soundShapeStart = expRandomBuffer[i + iSoundShapeStart*NUM_OVERTONES];
				float soundShapeEnd = expRandomBuffer[i + iSoundShapeEnd*NUM_OVERTONES];
				soundShape = relTimePoint * soundShapeEnd + (1.0f - relTimePoint) * soundShapeStart;

				if (i < maxOvertones)
				{
					// Modulation:
					float startMod = expRandomBuffer[i + (int)(fModulationPhase[instrument])*NUM_OVERTONES];
					float endMod = expRandomBuffer[i + (int)(fModulationPhase[instrument])*NUM_OVERTONES + NUM_OVERTONES];
					float modulation = modT * endMod + (1.0f - modT) * startMod;
					float fModulationAmount = floatInstParameter[instrument][K_MODULATION_AMOUNT];
					modulation = .5f + (modulation - 0.5f) * fModulationAmount;

					for (int j = 0; j < NUM_Stereo_VOICES; j++)
					{
						float fStereo = floatInstParameter[instrument][K_STEREO];
						outAmplitude[j] += (float)sin(fPhase[instrument][i][j] + fStereo * (2 * PI * randomBuffer[i*NUM_Stereo_VOICES + j])) *
							overtoneLoudness * soundShape * modulation;

						float fDetune = floatInstParameter[instrument][K_DETUNE];
						fPhase[instrument][i][j] += baseFreq * (i+1) * (1.0f + fStereo/64.0f * (randomBuffer[i] - 0.5f)) *
							 (1.0f + fDetune * (randomBuffer[i + iSoundShapeEnd*NUM_OVERTONES] - 0.5f));
						while (fPhase[instrument][i][j] > 2.0f * PI) fPhase[instrument][i][j] -= 2.0f * (float)PI;
					}
				}
				overallLoudness += overtoneLoudness * soundShape;
				float fQuakinessStart = floatInstParameter[instrument][K_QUAKINESS_START];
				float fQuakinessEnd = floatInstParameter[instrument][K_QUAKINESS_END];
				float quakiness = relTimePoint * fQuakinessEnd + (1.0f - relTimePoint) * fQuakinessStart;
				overtoneLoudness *= quakiness * 2.0f;
			}

			float fNoiseStart = floatInstParameter[instrument][K_NOISE_START];
			float fNoiseEnd = floatInstParameter[instrument][K_NOISE_END];
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
				reverbBuffer[instrument][reverbPos][j] = outAmplitude[j] * vol * fADSRVal[instrument];
			
				// Do the reverb feedback
				int fromBuffer = (j + 1) % NUM_Stereo_VOICES;
				int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[instrument][fromBuffer]) % 
					MAX_DELAY_LENGTH;
				float fDelayFeed = floatInstParameter[instrument][K_DELAY_FEED];
				reverbBuffer[instrument][reverbPos][j] += fDelayFeed * reverbBuffer[instrument][fromLocation][fromBuffer];
				if (fabsf(reverbBuffer[instrument][reverbPos][j]) < 1.0e-12) reverbBuffer[instrument][reverbPos][j] = 0.0f;
			}

#if 0
		*out1 = 0;
		*out2 = 0;
		for (int j = 0; j < NUM_Stereo_VOICES; j += 2)
		{
			//*out1 += outAmplitude[j];
			//*out2 += outAmplitude[j + 1];
			*out1 += reverbBuffer[reverbPos][j];
			*out2 += reverbBuffer[reverbPos][j+1];
		}
#endif


#if 1
			blockBuffer[sample*2] += (int)(4000 * reverbBuffer[instrument][reverbPos][0] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2] += (int)(4000 * reverbBuffer[instrument][reverbPos][2] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2+1] += (int)(4000 * reverbBuffer[instrument][reverbPos][1] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2+1] += (int)(4000 * reverbBuffer[instrument][reverbPos][3] * fADSRVal[instrument] * vol);
#else
			blockBuffer[sample*2] += (int)(8000 * outAmplitude[0] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2+1] += (int)(8000 * outAmplitude[1] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2] += (int)(8000 * outAmplitude[2] * fADSRVal[instrument] * vol);
			blockBuffer[sample*2+1] += (int)(8000 * outAmplitude[3] * fADSRVal[instrument] * vol);
#endif

			float fModulationSpeed = floatInstParameter[instrument][K_MODULATION_SPEED];
			fModulationPhase[instrument] += fModulationSpeed / 512.0f;
			fTimePoint[instrument] += SAMPLE_TICK_DURATION;
			if (fTimePoint[instrument] > fDuration) fTimePoint[instrument] = fDuration;
			while (fModulationPhase[instrument] > RANDOM_BUFFER_SIZE/2/NUM_OVERTONES) fModulationPhase[instrument] -= RANDOM_BUFFER_SIZE/2/NUM_OVERTONES;

			sampleID++;
		}			
	}

	startSampleID = sampleID;
}

// put here your synth
#pragma code_seg(".mzkInit")
void mzk_init()
{
	// init music data
	init_mzk_data();

#if 0
	// create some test output
	for (int block = 0; block < MZK_DURATION; block++)
	{
		// 2x due to stereo
		short *blockBuffer = buffer + 2 * block*MZK_BLOCK_SIZE;
		mzkPlayBlock(blockBuffer);
	}
#endif
}
