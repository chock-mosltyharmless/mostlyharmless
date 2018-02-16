//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

#include <math.h>
#include "mzk.h"
#include "music.h"
#include <emmintrin.h>
#include <fenv.h>
#include "xmmintrin.h"

#define MZK_BLOCK_SIZE AUDIO_BUFFER_SIZE

#ifndef PI
#define PI 3.1415f
#endif
//#define LOG_2_E 1.44269f
#define fScaler ((float)((double)2*PI / (double)MZK_RATE))

#define NUM_INSTRUMENTS 17
#define NUM_INSTRUMENT_PARAMETERS 35
// Number of additive overtones
#define NUM_OVERTONES 4

// Music time update parameter
#define SAMPLE_TICK_DURATION (1.0f / 32768.0f)

// 128 midi notes
#define kNumFrequencies 128
#if 0
static float freqtab[kNumFrequencies];
#endif

// The multiplier to get from midi int to float
#define MIDI_INT_TO_FLOAT (1.0f / 127.0f)

// Size of a buffer with random numbers
#define RANDOM_BUFFER_SIZE 65536
#define DELAY_MULTIPLICATOR 128
#define MAX_DELAY_LENGTH (DELAY_MULTIPLICATOR * 130) // Some safety for miscalculation stuff...

static float float_instrument_parameters_[NUM_INSTRUMENTS][NUM_INSTRUMENT_PARAMETERS];

static float lowNoise[RANDOM_BUFFER_SIZE];
//static int currentNoteIndex[NUM_INSTRUMENTS];
static int currentNote[NUM_INSTRUMENTS];
static float fPhase[NUM_INSTRUMENTS]; // Phase of the instrument

#define NUM_ADSR_DATA 5
static float adsrData[NUM_INSTRUMENTS][NUM_ADSR_DATA];
#define adsrVolume 0
#define adsrQuak 1
#define adsrDistort 2
#define adsrNoise 3
#define adsrDetune -1
#define adsrFreq 4

// MIDI volume
static int i_midi_volume_[NUM_INSTRUMENTS];

// attack values and stuff
static int iADSR[NUM_INSTRUMENTS];
static float fADSRVal[NUM_INSTRUMENTS];
static float floatOutput[MZK_BLOCK_SIZE][2];

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
#pragma code_seg(".jo_frand")
unsigned int jo_frand_seed_;
float JoFrand()
{
    unsigned long a = 214013;
    unsigned long c = 2531011;
    unsigned long m = 4294967296-1;
    jo_frand_seed_ = (jo_frand_seed_ * a + c) % m;
    //return (*s >> 8) % 65535;
    return (float)((jo_frand_seed_ >> 8) % 65535) * (1.0f/65536.0f);
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

#ifndef NO_INTRO_CODE
#pragma code_seg(".mzkPlayBlock")
void mzk_prepare_block(short *blockBuffer) {
#include "mzk_do.c"
}

// put here your synth
#pragma code_seg(".mzk_init")
void mzk_init() {
#include "mzk_init.c"
}
#endif