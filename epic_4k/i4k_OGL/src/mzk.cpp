//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"

static unsigned long seed;

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

// put here your synth
void mzk_init( short *buffer )
{
	int oldWaveVal = 0;

	// clear audio output to 0
	for (int k = 0; k < MZK_NUMSAMPLESC; k++)
	{
		// generator
		int data = (k) * ((k>>15) & (k>>12) & 63);
		// mono...
		int waveval = (data&511);
		if (data & 512) waveval = -waveval;
		data = (k/2) * (((k>>16) & (k>>13) + 4) & 127);
		int waveval2 = (data&1023);
		if (data & 1024) waveval2 = -waveval2;
		waveval += waveval2;

		// lowpass filter with gain
		waveval = 127 * oldWaveVal + waveval;
		oldWaveVal = waveval / 128;
		waveval /= 8;

		/* limiter */
		if (waveval > 16000) waveval = 16000;
		if (waveval < -16000) waveval = -16000;

		buffer[k] = (short)waveval;
	}

#if 0
	FILE *fid = fopen("waveout.raw", "wb");
	fwrite(buffer, sizeof(short), MZK_NUMSAMPLESC, fid);
	fclose(fid);
#endif
}
