//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#define NOISE_LENGTH (1<<20)
double noise[NOISE_LENGTH];
double outwave[MZK_NUMSAMPLES][2];

unsigned long seed;

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
	// Generate noise
	seed = 1;
	for (int samp = 0; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = frand() - 0.5f;
	}

	// filter the noise (very simple lowpass)
	for (int multiplier = 0; multiplier < 2; multiplier++)
	for (int samp = 1; samp < NOISE_LENGTH; samp++)
	{
		noise[samp] = 0.125f * noise[samp] + 0.9f * noise[samp-1];
	}
}

// put here your synth
void mzk_init( short *buffer )
{
	// init music data
	init_mzk_data();

	for (int k = 0; k < MZK_NUMSAMPLES; k++)
	{
		outwave[k][0] = (k*30) % 16000;
		outwave[k][1] = (k*30) % 16000;
	}

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

	// Save to file to be able to look at it at a later time.
#if 0
	FILE *fid = fopen("waveout.raw", "wb");
	fwrite(buffer, sizeof(short), MZK_NUMSAMPLESC, fid);
	fclose(fid);
#endif
}
