//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#define NOISE_LENGTH (1<<20)
#define WAVE_LENGTH ((NUM_FRAMES+128)*FRAME_STEP)

// The double amplitude interpolated from the amplitude
double d_amplitude[NUM_FRAMES][NUM_BANDS];
double noise[NOISE_LENGTH];
double wave[WAVE_LENGTH][2];
double outwave[MZK_NUMSAMPLES][2];

static unsigned long seed;

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
float frand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	//return (seed >> 8) % 65535;
	return (float)((seed>>8)%65535) * (1.0f/65536.0f);
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

// This method transforms the music data so that it can be used to generate samples
__inline void init_mzk_data()
{
	// Un-apply the rle compression of the pitch
	for (int i = 1; i < NUM_FRAMES; i++)
	{
		pitch[i] += pitch[i-1];
	}

	// Un-apply the rle compression of the amplitudes in frequency domain
	for (int i = 0; i < NUM_DEFINED_FRAMES; i++)
	{
		for (int band = 1; band < 12; band++)
		{
			amplitude[i][band] += amplitude[i][band-1];
		}
	}

	// Go to double domain
#if 0
	// This is not necessary, as the first frame is 0 anyways
	for (int band = 0; band < 12; band++)
	{
		d_amplitude[0][band] = (double)(amplitude[0][band]);
	}
#endif
	for (int band = 0; band < 12; band++)
	{
		int outpos = 1;
		for (int i = 1; i < NUM_DEFINED_FRAMES; i++)
		{
			double delta = (double)amplitude[i][band] / double(frameStep[i]+1);
			for (int j = 0; j < frameStep[i]+1; j++)
			{
				d_amplitude[outpos][band] = d_amplitude[(outpos-1)][band] + delta;
				outpos++;
			}
		}
	}

	// TODO: lots of unnecessary stuff here.
	// Generate noise
	seed = 1;
	for (int samp = 0; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = frand() - 0.5;
	}

	// filter the noise
	for (int multiplier = 0; multiplier < 2; multiplier++)
	for (int samp = 1; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = 0.125 * noise[samp] + 0.9 * noise[samp-1];
	}

	///////////////////////////
	// generate the main sample
	///////////////////////////

	// reset the random number generator
	seed = 1;
	int overtone = 1;
	for (int band = 0; band < NUM_BANDS; band++)
	{
		int shifter = band/2 - 1;
		if (shifter < 0) shifter = 0;
		int numBandTones = 1 << shifter;
		// Generate sound
		double curPitch = frand() * 3.1415926 * 2.0;
		for (int frame = 0; frame < NUM_FRAMES - 1; frame++)
		{
			double startPitch = (double)pitch[frame];
			double deltaPitch = (double)(pitch[frame+1] - startPitch) * (1.0/FRAME_STEP);
			double startAmplitude = d_amplitude[frame][band];
			double deltaAmplitude = (d_amplitude[(frame+1)][band] - startAmplitude) * (1.0/FRAME_STEP);
			for (int i = 0; i < FRAME_STEP; i++)
			{
				for (int bandTone = 0; bandTone < numBandTones; bandTone++)
				{	
					//double noisyness = (double)(band) * 0.125f;
					//double noisyness = (double)(band) * (0.0625f + 0.03125f);
					//double noisyness = (double)(overtone+bandTone) * (0.0625f);
					double noisyness = (double)(overtone+bandTone) * (0.0625f+0.03125f) - 0.5f;
					if (noisyness > 1.0) noisyness = 1.0;					
					if (noisyness < 0.0) noisyness = 0.0;
					double t = noisyness * 4. * noise[(i+FRAME_STEP*frame+16768*(overtone+bandTone))%NOISE_LENGTH] +
						       (1.0-noisyness);
					double output = sin(curPitch*(overtone+bandTone)) * t;
					wave[i+FRAME_STEP*frame][0] += exp2jo(startAmplitude) * output;
					wave[i+FRAME_STEP*frame][1] += exp2jo(startAmplitude) * output;
				}
				//double jitter = noise[(i+FRAME_STEP*frame)%NOISE_LENGTH] * 0.002;
				//double jitter = sin((i+FRAME_STEP*frame)*0.005) * 0.0004;
				double jitter = 0.0;
				curPitch += exp2jo(startPitch * (1.0/24) - 10.0) + jitter;
				startPitch += deltaPitch;
				startAmplitude += deltaAmplitude;
				while (curPitch > 2.*3.1415926) curPitch -= 2.*3.1415926;
			}
		}

		overtone += numBandTones;
	}

	// reverb
	for (int reverb = 0; reverb < 16; reverb++)
	{
		int delay = (int)frand() * 4096 + 2048;
		double inc1 = frand() * 0.25;
		double inc2 = 0.25 - inc1;
		if (frand() < 0.5)
		{
			inc1 = -inc1;
			inc2 = -inc2;
		}
		for (int samp = delay; samp < WAVE_LENGTH; samp++)
		{
			wave[samp][reverb&1] += inc1 * wave[samp-delay][(reverb/2)&1];
			wave[samp][reverb&1] += inc2 * wave[samp-delay+1][(reverb/2)&1];
		}
	}
}

// put here your synth
void mzk_init( short *buffer )
{
	// init music data
	init_mzk_data();

	// TODO: SCRIPT!

	int pos = 0;
	for (int k = 0; k < NUM_NOTES; k++)
	{
		pos += PATTERN_LENGTH*(int)(startTime[k]);
		double vol = exp2jo(volume[k]);
		int curPos = pos;
		int samplePos = INSTRUMENT_LENGTH*(int)(instrument[k]);

		for (int j = 0; j < INSTRUMENT_LENGTH; j++)
		{
			double waveval = vol * wave[j + samplePos][0];
			outwave[curPos][0] += waveval;
			waveval = vol * wave[j + samplePos][1];
			outwave[curPos][1] += waveval;

			curPos++;
		}

		pos += PATTERN_LENGTH;
	}

#if 0 // I scriptified
	// move base to output
	for (int k = 0; k < 30; k++)
	{
		double volume = 1.0;

		if (k < 8)
		{
			volume = k * 0.25 + 0.25;
		}

		int pos = k*(PATTERN_LENGTH);
		for (int j = 0; j < BASS_LENGTH; j++)
		{
			double waveval = volume * wave[j + BASS_START];

			outwave[pos][0] += waveval;
			outwave[pos][1] += waveval;

			pos++;
		}
	}

	// Add the first voice
	for (int k = 0; k < 4; k++)
	{
		int pos = (k+8)*PATTERN_LENGTH;
		for (int j = 0; j < BASS_LENGTH; j++)
		{
			double waveval = wave[j];

			outwave[pos][0] += waveval;
			outwave[pos][1] += waveval;

			pos++;
		}
	}

	// Add the second voice
	for (int k = 0; k < 8; k++)
	{
		int pos = (k+8+6)*PATTERN_LENGTH;
		if (k < 2 || k > 5)
		for (int j = 0; j < BASS_LENGTH; j++)
		{
			double waveval = wave[j+CODING_START];

			outwave[pos][0] += waveval;
			outwave[pos][1] += waveval;

			pos++;
		}
	}
#endif


#if 0
	// filter base
	for (int channel = 0; channel < 2; channel++)
	{
		double cutoff;
		double f, p, q;
		//q = 1.0f - 0.005; // 1. - cutoff
		for (int k = 0; k < 5; k++) filterY[k] = 0.0f;
	
		for (int samp = 0; samp < MZK_NUMSAMPLES; samp++)
		{
			if (samp < 2*PATTERN_LENGTH)
			{
				cutoff = 0.002;
			}
			else
			{
				//if (samp < 5 * PATTERN_LENGTH)
				//{
				//	cutoff = 0.2 * (samp-2*PATTERN_LENGTH) / 3*PATTERN_LENGTH + 0.002;
				//}
				//else
				{
					cutoff = 0.02;
				}
			}
			
			q = 1.0f - cutoff; // 1. - cutoff
			p = cutoff + 0.8f * cutoff * q; //cutoffFreq + 0.8f * cutoffFreq * q;
			f = p + p - 1.0f;
			q = 0.5;// / (cutoffFreq + 0.25);

			outwave[samp][channel] = filter(outwave[samp][channel], f, p, q, 0);
		}
	}
#endif

#if 1
	// move sample to output
	for (int k = 0; k < MZK_NUMSAMPLESC; k++)
	{
		// mono...
		int waveval = (int)outwave[0][k];
		
		/* limiter */
		if (waveval > 32767) waveval = 32767;
		if (waveval < -32767) waveval = -32767;

		buffer[k] = (short)waveval;
	}
#endif
}
