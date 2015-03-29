//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"
#include <emmintrin.h>

#ifndef PI
#define PI 3.1415f
#endif
#define LOG_2_E 1.44269f
#define fScaler ((float)((double)2*PI / (double)MZK_RATE))

#define NUM_INSTRUMENTS 5
#define NUM_INSTRUMENT_PARAMETERS 18
// Number of additive overtones
#define NUM_OVERTONES 16
#define NUM_Stereo_VOICES 4

// Music time update parameter
#define SAMPLE_TICK_DURATION (1.0f / 32768.0f)

// 128 midi notes
#define kNumFrequencies 128
#if 0
static float freqtab[kNumFrequencies];
#endif

// The multiplier to get from midi int to float
#define MIDI_INT_TO_FLOAT (1.0f / 127.0f)

// Size of a buffer with random numbers
#define RANDOM_BUFFER_SIZE 65536
#define DELAY_MULTIPLICATOR 128
#define MAX_DELAY_LENGTH (DELAY_MULTIPLICATOR * 130) // Some safety for miscalculation stuff...

static float randomBuffer[RANDOM_BUFFER_SIZE];
static float lowNoise[RANDOM_BUFFER_SIZE];
// The same as random Buffer, but contains exp(4*(randomBuffer-1))
static float expRandomBuffer[RANDOM_BUFFER_SIZE];
static float reverbBuffer[NUM_INSTRUMENTS][MAX_DELAY_LENGTH][NUM_Stereo_VOICES];
static int reverbBufferLength[NUM_Stereo_VOICES]; // Actual length taken for pull-out
// The note index that the instrument is on
static int currentNoteIndex[NUM_INSTRUMENTS];
static int currentNote[NUM_INSTRUMENTS];
static float fPhase[NUM_INSTRUMENTS][NUM_OVERTONES][NUM_Stereo_VOICES]; // Phase of the instrument
static float fTimePoint[NUM_INSTRUMENTS];
static float fModulationPhase[NUM_INSTRUMENTS];

// attack values and stuff
static int iADSR[NUM_INSTRUMENTS];
static float fADSRVal[NUM_INSTRUMENTS];

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

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
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
#if 0
	float k = 1.059463094359f;	// 12th root of 2
	float a = 6.875f;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (int i = 0; i < kNumFrequencies; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}
#endif

	// Make table with random number
	seed = 1;
	for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
	{
		randomBuffer[i] = frand();
		expRandomBuffer[i] = (float)exp2jo(LOG_2_E * 4.0f * (randomBuffer[i] - 1));
		lowNoise[i] = 16.0f * (randomBuffer[i] - 0.5f);
	}

#if 0
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
#endif

	// Ring-low-pass-filtering of lowPass
	// Use a one-pole
	float oldVal = 0.0f;
	for (int i = 0; i < RANDOM_BUFFER_SIZE * 8; i++)
	{
		lowNoise[i % RANDOM_BUFFER_SIZE] = 1.0f / 8.0f * lowNoise[i % RANDOM_BUFFER_SIZE] + (1.0f - 1.0f / 8.0f) * oldVal;
		oldVal = lowNoise[i % RANDOM_BUFFER_SIZE];
	}

	// Clear reverberation buffer
#if 0
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
#endif
}

#pragma code_seg(".mzkPlayBlock")
static float floatOutput[MZK_BLOCK_SIZE][2];
static float fInstParams[18];
void mzkPlayBlock(short *blockBuffer)
{
	static int startSampleID = 0;
	int sampleID;

	// clear audio block
	for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
	{
		// Stereo! --> 2 values
		floatOutput[0][sample] = 0;
	}

	for (int instrument = 0; instrument < NUM_INSTRUMENTS; instrument++)
	{
		// Go over all samples
		sampleID = startSampleID;

		// Set the reverberation buffer length at key start (no interpolation)
		reverbBufferLength[0] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR + 1;
		reverbBufferLength[1] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 7 / 17 + 1;
		reverbBufferLength[2] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 13 / 23 + 1;
		reverbBufferLength[3] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 11 / 13 + 1;

		// Check if we go to next note
		if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0)
		{
			if (savedNote[instrument][currentNoteIndex[instrument]] != -128)
			{
				// Key on
				iADSR[instrument] = 1; // attack
				fADSRVal[instrument] = 0.0f; // starting up
				fTimePoint[instrument] = 0.0f;

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
			}
			else
			{
				// NoteOff
				iADSR[instrument] = 3; // Release
			}

			// Go to next note location
			currentNoteIndex[instrument]++;
		}
		savedNoteTime[instrument][currentNoteIndex[instrument]]--;

		// Get audio frequency
		//float baseFreq = freqtab[currentNote[instrument] & 0x7f] * fScaler;
		float baseFreq = 8.175f * (float)exp2jo((float)currentNote[instrument] * (1.0f/12.0f)) * fScaler;
#if 0
		float fVolume = instrumentParams[instrument][F_VOLUME] * MIDI_INT_TO_FLOAT;
		float fAttack = instrumentParams[instrument][K_ATTACK] * MIDI_INT_TO_FLOAT;
		float fDecay = instrumentParams[instrument][K_DECAY] * MIDI_INT_TO_FLOAT;
		float fSustain = instrumentParams[instrument][K_SUSTAIN] * MIDI_INT_TO_FLOAT;
		float fRelease = instrumentParams[instrument][K_RELEASE] * MIDI_INT_TO_FLOAT;
		float fDuration = instrumentParams[instrument][K_DURATION] * MIDI_INT_TO_FLOAT;
		float fQuakinessStart = instrumentParams[instrument][K_QUAKINESS_START] * MIDI_INT_TO_FLOAT;
		float fQuakinessEnd = instrumentParams[instrument][K_QUAKINESS_END] * MIDI_INT_TO_FLOAT;
		float fModulationAmount = instrumentParams[instrument][K_MODULATION_AMOUNT] * MIDI_INT_TO_FLOAT;
		float fStereo = instrumentParams[instrument][K_STEREO] * MIDI_INT_TO_FLOAT;
		float fDetune = instrumentParams[instrument][K_DETUNE] * MIDI_INT_TO_FLOAT;
		float fNoiseStart = instrumentParams[instrument][K_NOISE_START] * MIDI_INT_TO_FLOAT;
		float fNoiseEnd = instrumentParams[instrument][K_NOISE_END] * MIDI_INT_TO_FLOAT;
		float fDelayFeed = instrumentParams[instrument][K_DELAY_FEED] * MIDI_INT_TO_FLOAT;
		float fModulationSpeed = instrumentParams[instrument][K_MODULATION_SPEED] * MIDI_INT_TO_FLOAT;
#else
		for (int i = 0; i < 18; i++) fInstParams[i] = instrumentParams[instrument][i] * MIDI_INT_TO_FLOAT;
#define fVolume fInstParams[F_VOLUME]
#define fAttack fInstParams[K_ATTACK]
#define fDecay fInstParams[K_DECAY]
#define fSustain fInstParams[K_SUSTAIN]
#define fRelease fInstParams[K_RELEASE]
#define fDuration fInstParams[K_DURATION]
#define fQuakinessStart fInstParams[K_QUAKINESS_START]
#define fQuakinessEnd fInstParams[K_QUAKINESS_END]
#define fModulationAmount fInstParams[K_MODULATION_AMOUNT]
#define fStereo fInstParams[K_STEREO]
#define fDetune fInstParams[K_DETUNE]
#define fNoiseStart fInstParams[K_NOISE_START]
#define fNoiseEnd fInstParams[K_NOISE_END]
#define fDelayFeed fInstParams[K_DELAY_FEED]
#define fModulationSpeed fInstParams[K_MODULATION_SPEED]
#endif

		for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++)
		{
			// Process ADSR envelope
			switch (iADSR[instrument])
			{
			case 1: // Attack
				fADSRVal[instrument] += fAttack / 256.0f;
				if (fADSRVal[instrument] > 1.0f)
				{
					iADSR[instrument] = 2;
					fADSRVal[instrument] = 1.0f;
				}
				break;
			case 2: // Decay
				fADSRVal[instrument] -= fSustain;
				fADSRVal[instrument] *= (1.0f - fDecay / 1024.0f);
				fADSRVal[instrument] += fSustain;
				break;
			case 3: // Release
				fADSRVal[instrument] *= (1.0f - fRelease / 1024.0f);
				break;
			default:
				break;
			}

#if 1
			// fade out on new instrument (but not on noteoff...)
			if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0 &&
				sample >= MZK_BLOCK_SIZE - 512 &&
			    savedNote[instrument][currentNoteIndex[instrument]] != -128)
			{
				fADSRVal[instrument] -= 1.0f / 512.0f;
			}
#endif

			// Some optimziation?
			if (fADSRVal[instrument] < 1.0f / 65536.0f) fADSRVal[instrument] = 0.0f;

			// The relative time point from instrument start to instrument end, modulation time
			float relTimePoint = fTimePoint[instrument] / (fDuration + 1.0f / 512.0f);
			float modT = fModulationPhase[instrument] - (float)ftoi_fast(fModulationPhase[instrument]);
					
			float outAmplitude[NUM_Stereo_VOICES] = {0};

			//int maxOvertones = ftoi_fast(3.0f / baseFreq);
			float overtoneLoudness = 1.0f;
			float overallLoudness = 0.0f;

#if 1
			if (instrument == 0)
			{
				// Hard-coded quak end envalope
				if (sampleID > MZK_BLOCK_SIZE * (4-4)) fQuakinessEnd = (float)(sampleID - MZK_BLOCK_SIZE * (4-4)) / (float)(MZK_BLOCK_SIZE * (70));;
				if (sampleID > MZK_BLOCK_SIZE * (64-4)) fQuakinessEnd = 0.f;
				if (sampleID > MZK_BLOCK_SIZE * (3*64+2-4)) fQuakinessEnd = (float)(sampleID - MZK_BLOCK_SIZE * (3*64+2-4)) / (float)(MZK_BLOCK_SIZE * (70));
				if (sampleID > MZK_BLOCK_SIZE * (4*64+2-4)) fQuakinessEnd = (float)(MZK_BLOCK_SIZE * (5*64-8-4) - sampleID) / (float)(MZK_BLOCK_SIZE * (70));
				if (sampleID > MZK_BLOCK_SIZE * (7*64-4)) fQuakinessEnd = (float)(sampleID - MZK_BLOCK_SIZE * (7*64-4)) / (float)(MZK_BLOCK_SIZE * (256)) + 0.25f;
				if (sampleID > MZK_BLOCK_SIZE * (9*64-4)) fQuakinessEnd = 0.7f;
				//if (sampleID > MZK_BLOCK_SIZE * (13*64-4)) fQuakinessEnd = (float)(sampleID - MZK_BLOCK_SIZE * (13*64-4)) / (float)(MZK_BLOCK_SIZE * (70));
				if (sampleID > MZK_BLOCK_SIZE * (13*64-4)) fQuakinessEnd = 0.f;
				if (fQuakinessEnd < 0.f) fQuakinessEnd = 0.f;
			}
#endif
			float quakiness = relTimePoint * fQuakinessEnd + (1.0f - relTimePoint) * fQuakinessStart;
			int modulationIndex = ftoi_fast(fModulationPhase[instrument])*NUM_OVERTONES;
			int iSoundShapeStart = instrumentParams[instrument][K_SOUND_SHAPE_START]*NUM_OVERTONES;
			int iSoundShapeEnd = instrumentParams[instrument][K_SOUND_SHAPE_END]*NUM_OVERTONES;
			float overtoneFreq = baseFreq;

			for (int i = 0; i < NUM_OVERTONES; i++)
			{
				float soundShapeStart = expRandomBuffer[iSoundShapeStart];
				float soundShapeEnd = expRandomBuffer[iSoundShapeEnd];
				float soundShape = relTimePoint * soundShapeEnd + (1.0f - relTimePoint) * soundShapeStart;

				//if (overtoneFreq < 3.f)
				{
					// Modulation:
					float startMod = expRandomBuffer[modulationIndex];
					float endMod = expRandomBuffer[modulationIndex + NUM_OVERTONES];
					float modulation = modT * endMod + (1.0f - modT) * startMod;
					modulation = .5f + (modulation - 0.5f) * fModulationAmount;

					for (int j = 0; j < NUM_Stereo_VOICES; j++)
					{
						outAmplitude[j] += (float)sin(fPhase[instrument][i][j] + fStereo * (2 * PI * randomBuffer[i*NUM_Stereo_VOICES + j])) *
							overtoneLoudness * soundShape * modulation;

						fPhase[instrument][i][j] += overtoneFreq * (1.0f + fStereo/64.0f * (randomBuffer[i] - 0.5f)) *
							 (1.0f + fDetune * (randomBuffer[iSoundShapeEnd] - 0.5f));
						while (fPhase[instrument][i][j] > 2.0f * PI) fPhase[instrument][i][j] -= 2.0f * (float)PI;
					}
				}
				overallLoudness += overtoneLoudness * soundShape;
				overtoneLoudness *= quakiness * 2.0f;
				overtoneFreq += baseFreq;
				modulationIndex++;
				iSoundShapeStart++;
				iSoundShapeEnd++;
			}

			// Put everything into the reverb buffers
			int reverbPos = sampleID % MAX_DELAY_LENGTH;
			float noiseAmount = relTimePoint * fNoiseEnd + (1.0f - relTimePoint) * fNoiseStart;
			float totalLoudness = 4.f;
			if (sampleID > MZK_BLOCK_SIZE * (18*64-4)) totalLoudness -= (float)(sampleID - MZK_BLOCK_SIZE * (18*64-4)) / ((float)MZK_BLOCK_SIZE * 24);
			if (totalLoudness < 0.f) totalLoudness = 0.f;
			for (int j = 0; j < NUM_Stereo_VOICES; j++)
			{
				// Adjust volume
				float ampl = outAmplitude[j] / overallLoudness;

				// Ring modulation with noise
				ampl *= 1.0f + (lowNoise[sampleID % RANDOM_BUFFER_SIZE] - 1.0f) * noiseAmount;

				reverbBuffer[instrument][reverbPos][j] = totalLoudness * ampl * fVolume * fADSRVal[instrument];
			
				// Do the reverb feedback
				int fromBuffer = (j + 1) % NUM_Stereo_VOICES;
				int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[fromBuffer]) % 
					MAX_DELAY_LENGTH;
				reverbBuffer[instrument][reverbPos][j] += fDelayFeed * reverbBuffer[instrument][fromLocation][fromBuffer];
				// Ignore denormals..
				//if (fabsf(reverbBuffer[instrument][reverbPos][j]) < 1.0e-12) reverbBuffer[instrument][reverbPos][j] = 0.0f;
			}

			floatOutput[sample][0] += reverbBuffer[instrument][reverbPos][0];
			floatOutput[sample][0] += reverbBuffer[instrument][reverbPos][2];
			floatOutput[sample][1] += reverbBuffer[instrument][reverbPos][1];
			floatOutput[sample][1] += reverbBuffer[instrument][reverbPos][3];

			fModulationPhase[instrument] += fModulationSpeed / 512.0f;
			fTimePoint[instrument] += SAMPLE_TICK_DURATION;
			if (fTimePoint[instrument] > fDuration) fTimePoint[instrument] = fDuration;
			while (fModulationPhase[instrument] > RANDOM_BUFFER_SIZE/2/NUM_OVERTONES) fModulationPhase[instrument] -= RANDOM_BUFFER_SIZE/2/NUM_OVERTONES;

			sampleID++;
		}
	}

	// Copy to int output
	for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
	{
#if 0
		int val = (int)floatOutput[0][sample];
		if (val > 32767) val = 32767;
		if (val < -32767) val = -32767;
		blockBuffer[sample] = val;
#else
		float val = floatOutput[0][sample];
		if (val > 1.5f) val = 1.5f;
		if (val < -1.5f) val = -1.5f;
		val = (float)sin(val) * 32768.0f;
		blockBuffer[sample] = ftoi_fast(val);
#endif
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
