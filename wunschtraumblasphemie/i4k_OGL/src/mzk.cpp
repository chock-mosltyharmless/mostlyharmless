//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <MMSystem.h>

#include <math.h>
#include "mzk.h"
#include "music.h"

static double outwave[AUDIO_BUFFER_SIZE][2];

// Music data
#define MAX_NUM_INSTRUMENTS 12
// TODO: combine frequency and phasestep and work in phaseStep domain directly.
static float phaseStep[MAX_NUM_INSTRUMENTS];
static float amplitude[MAX_NUM_INSTRUMENTS];
static float phase[2][MAX_NUM_INSTRUMENTS]; // for stereo, not for next-note
const int sceneDuration = 300000; // will be able to change over time
static int sceneTime = (2*AUDIO_BUFFER_SIZE); // The time inside the scene
// For createNextNote
static unsigned int sceneSeed = 0;
static int sceneID = 0;

// The grains
static int grainPos[2][MAX_NUM_INSTRUMENTS]; // position inside the grain
static int grainLength[2][MAX_NUM_INSTRUMENTS]; // Length of the grain until restart
static float grainPhase[2][MAX_NUM_INSTRUMENTS][2]; // phase of the output stuff (2 for detune)
static float grainPhaseStep[2][MAX_NUM_INSTRUMENTS];
static float grainAmplitude[2][MAX_NUM_INSTRUMENTS]; // current amplitude

// create random value between -65534 and 65534?
/*int rand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	return (seed >> 8) % 65535;
}*/

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
float frand(unsigned int *seed)
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	*seed = ((*seed) * a + c) % m;
	//return (seed >> 8) % 65535;
	return (float)(((*seed)>>8)%65535) * (1.0f/65536.0f);
}

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

#include <stdio.h>

void createNextNotes()
{
	sceneID++;

	// I generate one sound only here statically.
	// phaseStep stores the frequency at first.
	//phaseStep[nextInstrumentBank][0] = 400.0f;
	//amplitude[0] = 8192.0f;

	//float baseFreq = (200.0f * 2.0f * 3.1415926f / 44100.0f) *
	//	(int)(frand(&sceneSeed) * 6.0f + 3.0f) / 5.0f;
	float baseFreq = (400.0f * 2.0f * 3.1415926f / 44100.0f) *
		(int)(frand(&sceneSeed) * 2.5f + 1.0f) * 2.0f / 5.0f;

	//float multiplicator = (int)(frand(&sceneSeed)* 1.5f) / 2.0f + 1.0f;
	for (int inst = 0; inst < MAX_NUM_INSTRUMENTS; inst++)
	{
		phaseStep[inst] = baseFreq / 5.0f * (inst + (int)(frand(&sceneSeed)*8.0f));
		//phaseStep[inst] = baseFreq / 5.0f * (int)(multiplicator * (inst+1));
		amplitude[inst] = 4096.0f / (phaseStep[inst] + 0.125f);
	}
}

// This method transforms the music data so that it can be used to generate samples
void mzk_init()
{
	sceneSeed = timeGetTime(); // Music based on time
	createNextNotes();
}

// put here your synth
void mzk_prepare_block(short *buffer)
{
	for (unsigned int k = 0; k < AUDIO_BUFFER_SIZE; k++)
	{
		outwave[k][0] = 0.0f;
		outwave[k][1] = 0.0f;

		for (int channel = 0; channel < 2; channel++)
		{
			for (int inst = 0; inst < MAX_NUM_INSTRUMENTS && inst < 2*sceneID; inst++)
			{
				if (grainPos[channel][inst] >= grainLength[channel][inst])
				{
					grainPos[channel][inst] = 0;
					grainLength[channel][inst] = 16000 + (int)(frand(&sceneSeed) * 16768.0f);
					grainPhase[channel][inst][0] = 0.0f;
					grainPhase[channel][inst][1] = 0.0f;
					grainPhaseStep[channel][inst] = phaseStep[inst];
					grainAmplitude[channel][inst] = amplitude[inst];
				}

				outwave[k][channel] +=
					(sin(grainPhase[channel][inst][0]) + sin(grainPhase[channel][inst][1])) *
					grainAmplitude[channel][inst];
				grainPhase[channel][inst][0] += grainPhaseStep[channel][inst] * 1.01f;
				grainPhase[channel][inst][1] += grainPhaseStep[channel][inst] * 0.99f;
				grainPhaseStep[channel][inst] = phaseStep[inst] + (65536.0f*4.0f + grainPos[channel][inst]) / 65536.0f / 4.0f / 48.0f;
				if (grainPos[channel][inst] < 256) grainAmplitude[channel][inst] = amplitude[inst] * grainPos[channel][inst] / 256.0f;
				else grainAmplitude[channel][inst] = amplitude[inst];
				grainAmplitude[channel][inst] /= (64.0f + grainPos[channel][inst]) / 64.0f;
				if (grainAmplitude[channel][inst] < 0.0f) grainAmplitude[channel][inst] = 0.0f;

				grainPos[channel][inst]++;
			}
		}

		// move time forward and check whether notes have to be regenerated.
		sceneTime++;
		if (sceneTime == sceneDuration)
		{
			sceneTime -= sceneDuration;
			createNextNotes();
		}

	}

	// move sample to output
	for (int k = 0; k < AUDIO_BUFFER_SIZE*MZK_NUMCHANNELS; k++)
	{
		// mono... (no, stereo)
		int waveval = (int)outwave[0][k];
		
		/* limiter */
		if (waveval > 32767) waveval = 32767;
		if (waveval < -32767) waveval = -32767;

		buffer[k] = (short)waveval;
	}

	// Save to file to be able to look at it at a later time.
#if 0
	FILE *fid = fopen("waveout.raw", "wb");
	fwrite(buffer, sizeof(short), AUDIO_BUFFER_SIZE * MZK_NUMCHANNELS, fid);
	fclose(fid);
#endif
}
