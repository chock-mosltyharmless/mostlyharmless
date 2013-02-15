#include "Mathhelpers.h"

static unsigned long seed;

int jrand()
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	unsigned long retval = (seed >> RAND_SHIFT);
	return ((int)retval % MAX_RAND_INT);
}

float fjrand()
{
	return (float)(jrand() * (1.0f / MAX_RAND_INT));
}
