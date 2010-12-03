//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#define STEP_SIZE 512
#define LOG_STEP_SIZE 9
#define PI 3.1415

static unsigned long seed;

// const data for music
#pragma data_seg(".melBins")
const static int melSumBins = 360;
const static unsigned char melNumBins[NUM_FILTERS] = {
	3, 3, 4, 4, 5, 6, 6, 8, 11,
	12, 17, 23, 33, 43, 71, 111
};

// temporary data for FFT
static int realData[STEP_SIZE*2];
int realData2[melSumBins];
int imagData2[melSumBins];
static int tmpBuffer[257*STEP_SIZE];
static int tmpBuffer2[257*STEP_SIZE];
static int tmpBuffer3[2*MZK_NUMSAMPLES];
static int cosTable[2 * STEP_SIZE];

/* Reverberation data */
int reverbPos = 0;			// the current position in reverb shita...
int reverbLine[2048][2];
int reverbTakeofs[3][2] = {{611, 327}, {721, 451}, {1528, 1612}};

extern short myMuzik[];

/* function parameters */
int *input;
int *output;
int logLength;
int pitch;


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

#pragma code_seg(".invWaveletTransform")
static void invWaveletTransform()
{
	int length = (1<<logLength);

	// mean output:
	for (int i = 0; i < length; i++)
	{
		output[i*pitch] = 2 * input[0];
	}

	int inputPos = pitch;
	for (int clength = length; clength > 1; clength >>= 1)
	{
		int curPos = 0;
		for (int numStuff = 0; numStuff < length/clength; numStuff++)
		{			
			// subtract it from the input
			for (int cl = 0; cl < clength; cl++)
			{
				output[curPos] += input[inputPos] * 2 * (2 * cl - clength + 1) / clength;
				curPos += pitch;
			}

			inputPos += pitch;
		}
	}
}

// This method transforms the music data so that it can be used to generate samples
#pragma code_seg(".initMzkData")
__inline void init_mzk_data()
{
	// Un-apply the rle compression
	int outPos = 0;
	for (int i = 0; i < sizeof(rleValues); i++)
	{
		musicData[outPos] = (int)(rleValues[i]);
		outPos += (int)(rleZeroes[i]) + 1;
	}

	// Inverse wavelet transform
	int *source1 = wavelet.energies;
	int *source2 = wavelet.voicedness;
	int *dest1 = energiesTMP;
	int *dest2 = voicednessTMP;
	for (int sample = 0; sample < NUM_SAMPLES; sample++)
	{
		for (int i = 0; i < NUM_FILTERS; i++)
		{
			input = source1+i;
			output = dest1+i;
			logLength = logSampleFrames[sample];
			pitch = NUM_FILTERS;
			invWaveletTransform();
			input = source2+i;
			output = dest2+i;
			invWaveletTransform();
		}
		for (int i = 0; i < 1<<logSampleFrames[sample]; i++)
		{
			input = dest1+i*NUM_FILTERS;
			output = source1+i*NUM_FILTERS;
			logLength = LOG_NUM_FILTERS;
			pitch = 1;
			invWaveletTransform();
			input = dest2+i*NUM_FILTERS;
			output = source2+i*NUM_FILTERS;
			invWaveletTransform();
		}

		source1 += (1<<logSampleFrames[sample])*NUM_FILTERS;
		source2 += (1<<logSampleFrames[sample])*NUM_FILTERS;
		dest1 += (1<<logSampleFrames[sample])*NUM_FILTERS;
		dest2 += (1<<logSampleFrames[sample])*NUM_FILTERS;
	}

	/* Go to max 63 voicedness */
	/*for (int sample = 0; sample < NUM_SAMPLES*NUM_FILTERS; sample++)
	{
		if (wavelet.voicedness[sample] < 0) wavelet.voicedness[sample] = 0;
		if (wavelet.voicedness[sample] > 63) wavelet.voicedness[sample] = 63;
	}*/

	// Undo the diffmarker magic
	for (int i = 1; i < SUM_SAMPLE_FRAMES; i++)
	{
		wavelet.diffMarkers[i] += wavelet.diffMarkers[i-1];
	}
}

// triangle window whatever.
#pragma code_seg(".triangleWindow")
int triangleWindow(int x)
{
	/* new algorithm... */
	if (x <= STEP_SIZE)
	{
		return 128 - (cosTable[x]>>4);
	}
	else
	{
		return 128 + (cosTable[x-STEP_SIZE]>>4);
	}
}

// create the cosinus table
#pragma code_seg(".createCosTable")
void createCosTable()
{
	for (int i = 0; i < 2 * STEP_SIZE; i++)
	{
		//cosTable[i] = (int)(128.0 * cos(2.0 * PI * i / (2 * STEP_SIZE)));
		cosTable[i] = (int)(128.0 * 16.0 * cos(2.0 * PI * i / (2 * STEP_SIZE)));
	}
}

// This function does the actual filtering...
// The idea with the voiced flag sounds stupid to me.
// TODO: If I use something different than 256 for the sin, I get something like a chorus..
#pragma code_seg(".melDFTFilter")
void melDFTFilter(int *inBuffer, int *outBuffer, int *energies, int *voicedness, int voiced)
{
	for (int k = 0; k < 2 * STEP_SIZE; k++)
	{
		// TODO: I might not need the shift?
		realData2[k] = 0;
		imagData2[k] = 0;
	}

	int outPos = 0;
	for (int filter = 0; filter < NUM_FILTERS; filter++)
	{
		int vs = voicedness[filter];
		if (!voiced) vs = 64 - vs;
		int filterEner = (int)(vs * exp2jo((energies[filter] - 160) * (1.0 / 16.0)));

		for (int bin = 0; bin < melNumBins[filter] + 1; bin++)
		{
			int real = 0, imag = 0;
			for (int j = 0; j < 2 * STEP_SIZE; j++)
			{
				real += (cosTable[(outPos * j) & (2 * STEP_SIZE - 1)]>>4) * inBuffer[j] * triangleWindow(j);
				imag += (cosTable[(outPos * j + 280) & (2 * STEP_SIZE - 1)]>>4) * inBuffer[j] * triangleWindow(j);
			}
			if (bin == 0 || bin == melNumBins[filter])
			{
				realData2[outPos] += (real >> 16) * filterEner / 2;
				imagData2[outPos] += (imag >> 16) * filterEner / 2;
			}
			else
			{
				realData2[outPos] += (real >> 16) * filterEner;
				imagData2[outPos] += (imag >> 16) * filterEner;
			}
			outPos++;
		}
		outPos--;
	}

	for (int i = 0; i < 2 * STEP_SIZE; i++)
	{
		int real = 0;
		for (int j = 0; j < melSumBins; j++)
		{
			real += ((cosTable[(i * j) & (2 * STEP_SIZE - 1)]>>4) * realData2[j]) >> 4;
			real += ((cosTable[(i * j + 280) & (2 * STEP_SIZE - 1)]>>4) * imagData2[j]) >> 4;
		}
		
		outBuffer[i] += real >> 12;
	}
}

// Generate pitch train
// TODO: I need to something with the last half-frame!
// TODO: I think this can be simplified, no?
#pragma code_seg(".pitchTrain")
void pitchTrain(int *diffMarkers, int numSamples)
{
	unsigned int t = 0;
	int value, oldValue = 128 * 256;
	for (int samplePos = 0; samplePos < numSamples; samplePos++)
	{
		int frame = samplePos >> (LOG_STEP_SIZE);
		t += (int)((STEP_SIZE * exp2jo(-(((float)(diffMarkers[frame]+72))/24.0))) * (1<<22));
		value = abs(cosTable[(t + (256<<22)) >> 22] - cosTable[t>>22]);
		tmpBuffer[samplePos] = value - oldValue;
		oldValue = value;
	}
}



// put here your synth
#pragma code_seg(".mzkInit")
// put here your synth
void mzk_init()
{
	short *buffer = myMuzik+22;
	int sample = 0; // only sample 0...
	int samplePos = 0;
		
	// init music data
	init_mzk_data();
	// create bitreverse table for FFT calculation
	//createBitreverseTable();
	createCosTable();

	for (int sample = 0; sample < NUM_SAMPLES; sample++)
	{
		int frameV = 0;
		int frameU = 0;

		// generate voiced whatever.
		pitchTrain(wavelet.diffMarkers + samplePos, (1<<logSampleFrames[sample])*STEP_SIZE);

		// clear outputbuffer:
		for (int k = 0; k < 257*STEP_SIZE; k++)
		{
			tmpBuffer2[k] = 0;
		}

		// filter voiced:
		for (int k = 0; k < (1<<logSampleFrames[sample]); k++)
		{
			// filter using depacked energies:
			melDFTFilter(tmpBuffer+k*STEP_SIZE, tmpBuffer2+k*STEP_SIZE,
						wavelet.energies + NUM_FILTERS*frameV + NUM_FILTERS*samplePos,
						wavelet.voicedness + NUM_FILTERS*frameV + NUM_FILTERS*samplePos, 1);
			frameV++;
		}

		// generate unvoiced whatever.
		for (int k = 0; k < 257*STEP_SIZE; k++)
		{
			tmpBuffer[k] = (rand() >> 8) - 127;
		}

		// filter unvoiced:
		for (int k = 0; k < (1<<logSampleFrames[sample]); k++)
		{
			// filter using depacked energies:
			melDFTFilter(tmpBuffer+k*STEP_SIZE, tmpBuffer2+k*STEP_SIZE,
				         wavelet.energies + NUM_FILTERS*frameU + NUM_FILTERS*samplePos,
						 wavelet.voicedness + NUM_FILTERS*frameU + NUM_FILTERS*samplePos, 0);
			frameU++;
		}

		// move sample0 to output:
		int pos = rand(); /* random, rather */
		while (pos < MZK_NUMSAMPLES - 257*STEP_SIZE - 3*44100)
		{
			for (int k = 0; k < 257*STEP_SIZE; k++)
			{
				/* limiter + reverb + DC-BLOCKER! ? */
				tmpBuffer3[(sample&1) + 2*(pos + k)] += (tmpBuffer2[k] * ((pos) / 16768) / 64);
			}
	
			//pos += (pos + 64000) / 8192 * (rand()+32768) / 1024;
			pos += (pos + 640000) / 1024 * (rand()+32768) / 1024;
		}

		samplePos += (1<<logSampleFrames[sample]);
	}

	/* Limiter & NO DC blocker & Reverb */
	for (int i = 0; i < 2*MZK_NUMSAMPLES; i++)
	{
		int rev = 0;
		for (int k = 0; k < 3; k++)
		{				
			rev += reverbLine[(reverbPos + reverbTakeofs[k][i&1])&2047][((k+i+1)&1)]
				 + reverbLine[(reverbPos + reverbTakeofs[k][i&1]+1)&2047][((k+i+1)&1)];
		}		
		reverbLine[reverbPos][i&1] = tmpBuffer3[i] + rev / 8;
		//tmpBuffer3[i] = rev;		
		if (rev > 32767) rev = 32767;
		if (rev < -32767) rev = -32767;
		buffer[i] = (short)rev;
		
		reverbPos += 2048 - (i&1);
		reverbPos &= 2047;
	}

	/* Limiter & DC blocker */
#if 0
	for (int i = 2; i < 2*MZK_NUMSAMPLES; i++)
	{	
		//y = tmpBuffer3[i] - oldx[i&1] + 15*tmpBuffer3[i-2]/16;
		//oldx[i&1] = tmpBuffer3[i];
		y = tmpBuffer3[i];
		if (y > 32767) y = 32767;
		if (y < -32767) y = -32767;
		buffer[i] = (short)y;
	}
#endif

#if 0
	for (int k = 0; k < MZK_NUMSAMPLES && k < 257*STEP_SIZE; k++)
	{
		/* limiter */
		if (tmpBuffer2[k] > 32767) tmpBuffer2[k] = 32767;
		if (tmpBuffer2[k] < -32767) tmpBuffer2[k] = -32767;

		buffer[2*k] = (short)(tmpBuffer2[k]); // in destination not necessary due to loudness?		
		buffer[2*k+1] = (short)(tmpBuffer2[k]); // in destination not necessary due to loudness?		
	}
#endif	
}
