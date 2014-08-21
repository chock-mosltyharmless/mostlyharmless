#include "stdafx.h"
#include "global.h"

unsigned int blr_rand(unsigned int seed)
{
	unsigned long a = 214013;
	unsigned long c = 2531011;
	unsigned long m = 4294967296-1;
	seed = (seed * a + c) % m;
	return seed;
}
