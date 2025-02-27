//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#define NOISE_LENGTH (1<<20)
#define WAVE_LENGTH ((NUM_FRAMES+128)*FRAME_STEP) * 3 /* Number of down pitches */

// The double amplitude interpolated from the amplitude
double d_amplitude[NUM_FRAMES][NUM_BANDS];
double noise[NOISE_LENGTH];
double wave[WAVE_LENGTH][2];
double outwave[MZK_NUMSAMPLES + FRAME_STEP*300][2];

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

#include <stdio.h>

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
		for (int band = 1; band < NUM_BANDS; band++)
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
	for (int band = 0; band < NUM_BANDS; band++)
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
		noise[samp] = frand() - 0.5f;
	}

	// filter the noise
	for (int multiplier = 0; multiplier < 2; multiplier++)
	for (int samp = 1; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = 0.125f * noise[samp] + 0.9f * noise[samp-1];
	}

	///////////////////////////
	// generate the main sample
	///////////////////////////

	for (int downPitch = 0; downPitch < 3; downPitch++)
	{
		//for (int singer = 0; singer < 3; singer++)
		int singer = 0;
		{
			// reset the random number generator
			seed = singer;

			// Modify the data for the individual singers:
#if 0
			for (int i = 0; i < NUM_FRAMES; i++)
			{
				pitch[i] += (int)((frand()-0.5f)*3.0f);
			}
			for (int i = 0; i < NUM_DEFINED_FRAMES*NUM_BANDS; i++)
			{
				d_amplitude[0][i] += (int)((frand()-0.5f)*2.0f);
			}
#endif

			int overtone = 1;
			for (int band = 0; band < NUM_BANDS; band++)
			{
				int shifter = band/2;
				int numBandTones = 1 << shifter;
				// Generate sound
				double curPitch = frand() * 3.1415926f * 2.0f;
				for (int frame = 0; frame < NUM_FRAMES - 1; frame++)
				{
					double startPitch = (double)(pitch[frame] + downPitch*12); // downpitching happens here
					double deltaPitch = (double)(pitch[frame+1] - pitch[frame]) * (1.0f/FRAME_STEP);
					double startAmplitude = d_amplitude[frame][band];
					double deltaAmplitude = (d_amplitude[(frame+1)][band] - startAmplitude) * (1.0f/FRAME_STEP);
					for (int i = 0; i < FRAME_STEP; i++)
					{
						for (int bandTone = 0; bandTone < numBandTones; bandTone++)
						{	
							double noisyness = (double)(overtone+bandTone) * (0.0625f+0.03125f) - 0.5f;
							if (noisyness > 1.0f) noisyness = 1.0f;					
							if (noisyness < 0.0f) noisyness = 0.0f;
							double t = noisyness * 4.f * noise[(i+FRAME_STEP*frame+16768*(overtone+bandTone))%NOISE_LENGTH] +
									   (1.0f-noisyness);
							double output = sin(curPitch*(overtone+bandTone)) * t * 0.25f * exp2jo(startAmplitude);
							wave[i+FRAME_STEP*frame + downPitch*FRAME_STEP*NUM_FRAMES][0] += output;
							wave[i+FRAME_STEP*frame + downPitch*FRAME_STEP*NUM_FRAMES][1] += output;
						}
						curPitch += exp2jo(startPitch * (1.0f/24) - 4.0f);
						startPitch += deltaPitch;
						startAmplitude += deltaAmplitude;
						while (curPitch > 2.f*3.1415926f) curPitch -= 2.f*3.1415926f;
					}
				}

				overtone += numBandTones;
			}
		}
	}

	// reverb
#if 0
	for (int reverb = 0; reverb < 16; reverb++)
	{
		int delay = (int)frand() * 4096 + 2048;
		//int delay = (int)frand() * 2048 + 1024;
		double inc1 = frand() * 0.125;
		double inc2 = 0.125 - inc1;
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
#endif
}

// put here your synth
void mzk_init( short *buffer )
{
	// init music data
	init_mzk_data();

#if 1
	for (int singer = 0; singer < 3; singer++)
	{
		// Three strophes
		//for (int strophe = 0; strophe < 3 - singer; strophe++)
		{
			//int stropheStart = strophe * STROPHE_DURATION * FRAME_STEP + singer*SECOND_SINGER_WAIT*FRAME_STEP;
			int stropheStart = singer*SECOND_SINGER_WAIT*FRAME_STEP;
			// Render a single strophe
			for (int k = 0; k < sizeof(noteInstruments); k++)
			{
				int sampleStart = (noteInstruments[k]+singer*NUM_INSTRUMENTS) * INSTRUMENT_LEN * FRAME_STEP;
				int outputStart = k * INSTRUMENT_DURATION * FRAME_STEP + stropheStart;
				for (int i = 0; i < INSTRUMENT_LEN * FRAME_STEP * 2 /*channels*/; i++)
				{
					outwave[outputStart][i] += wave[sampleStart][i];
				}
			}
		}
	}

#else // This was just for testing the samples
	for (int k = 0; k < (NUM_FRAMES - 1) * FRAME_STEP * 2 /* pitchers */; k++)
	{
		outwave[k][0] = wave[k][0];
		outwave[k][1] = wave[k][1];
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

#if 0
	FILE *fid = fopen("waveout.raw", "wb");
	fwrite(buffer, sizeof(short), MZK_NUMSAMPLESC, fid);
	fclose(fid);
#endif
}
