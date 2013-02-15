#pragma once

#define MAX_RAND_INT (1<<24)
// Make sure that MAX_RAND_INT >> RAND_SHIFT does not overflow.
#define RAND_SHIFT (6)

// returns a number in [0, MAX_RAND_INT[
int jrand();

// returns a float in [0, 1[
float fjrand();
