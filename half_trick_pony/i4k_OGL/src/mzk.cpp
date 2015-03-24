//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include "music.h"

#ifndef PI
#define PI 3.1415
#endif

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
	// Un-apply delta compression
}

// put here your synth
#pragma code_seg(".mzkInit")
void mzk_init( short *buffer )
{
	// init music data
	init_mzk_data();

	// create some test output
	for (int k = 0; k < MZK_NUMSAMPLES; k++)
	{
		buffer[2*k] = (short)(k % 128) * 32; // in destination not necessary due to loudness?		
		buffer[2*k+1] = (short)(k % 96) * 32; // in destination not necessary due to loudness?		
	}
}
