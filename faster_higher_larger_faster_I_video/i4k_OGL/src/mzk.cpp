//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#include <math.h>
#include "mzk.h"
#include <emmintrin.h>

#ifdef WRITE_MUSIC
#include <stdio.h>
#endif

#ifndef PI
#define PI 3.1415f
#endif
#define LOG_2_E 1.44269f
#define fScaler ((float)((double)2*PI / (double)MZK_RATE))

// Size of a buffer with random numbers
#define RANDOM_BUFFER_SIZE 65536

// Accumulated volume of the drums (wraps at 157.1797414240835)
double accumulated_drum_volume = 0.0;
bool has_ended = false;

static float randomBuffer[RANDOM_BUFFER_SIZE];
static float lowNoise[RANDOM_BUFFER_SIZE];
// The same as random Buffer, but contains exp(4*(randomBuffer-1))
static float expRandomBuffer[RANDOM_BUFFER_SIZE];

static unsigned long seed;

#pragma code_seg(".rand")
void srand(unsigned int new_seed) {
    seed = new_seed;
}

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

inline int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
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

static int startSampleID = 0;
static float fdrum_position = 0;
static float last_loudness = 1.0f;
static int last_drum_position = 0;
static int skipped_drum_position = 0;
static float kMinBaseFrequency = 200.0f;
// Start "in the middle" to offset beat and tone reset
static float base_frequency = kMinBaseFrequency * 1.5f;
static float base_offset = 0.0f;

#pragma code_seg(".mzkPlayBlock")
static float floatOutput[MZK_BLOCK_SIZE][2];
void mzkPlayBlock(short *blockBuffer)
{
    float cur_accumulated_drum_volume = 0.0f;

	// clear audio block
	for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
	{
		// Stereo! --> 2 values
		floatOutput[0][sample] = 0;
	}

    int sampleID = startSampleID;
    float loudness = last_loudness;

    for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++) {
        const int kMinDrumStep = 64;
        const int kDrumStep = 512 * kMinDrumStep;

        float speedup = 0.5f;
        if (speedup > 0.5f) speedup = 0.5f;
        speedup += (float)((last_drum_position + skipped_drum_position) % (kDrumStep*4)) / (kDrumStep*4) / 2;
        if ((last_drum_position + skipped_drum_position) >= (kDrumStep*4)) {
            int want_to_know = sampleID;
        }
        float multiplier = 1.0f / speedup;
        //float total_speed = (float)(sampleID) / (1<<22);
        float total_speed = 0.40603695f;  // Optimized to repeat the same as music.

        //total_speed = 0.5f;
        fdrum_position += speedup * total_speed;
        if (fdrum_position > kDrumStep) {
            fdrum_position -= kDrumStep;
            skipped_drum_position += kDrumStep;
        }

        int drum_position = ftoi_fast(fdrum_position);

        // Count ones before the first zero
        // I may do this more intelligent by divinding by 2 given some stuffs
        float drum_loudness = 0.0f;
        float log_drum_loudness = 0.0f;
        if (drum_position != last_drum_position) {
            if (!(drum_position & (2 * kMinDrumStep - 1))) {
                if (drum_position & (2 * kMinDrumStep)) {
                    if (drum_position & (4 * kMinDrumStep)) {
                        if (drum_position & (8 * kMinDrumStep)) {
                            if (drum_position & (16 * kMinDrumStep)) {
                                if (drum_position & (32 * kMinDrumStep)) {
                                    if (drum_position & (64 * kMinDrumStep)) {
                                        if (drum_position & (128 * kMinDrumStep)) {
                                            if (drum_position & (256 * kMinDrumStep)) {
                                                drum_loudness = 1.0f;
                                                log_drum_loudness = 9.0f;
                                            } else {
                                                drum_loudness = (0.5f) * multiplier;
                                                log_drum_loudness = 7.0f + multiplier;
                                            }
                                        } else {
                                            drum_loudness = (0.25f) * multiplier;
                                            log_drum_loudness = 6.0f + multiplier;
                                        }
                                    } else {
                                        drum_loudness = (0.125f) * multiplier;
                                        log_drum_loudness = 4.0f + multiplier;
                                    }
                                } else {
                                    drum_loudness = (0.0625f) * multiplier;
                                    log_drum_loudness = 2.0f + multiplier;
                                }
                            } else {
                                drum_loudness = (0.03125f) * multiplier;
                                log_drum_loudness = 1.0f + multiplier;
                            }
                        } else {
                            drum_loudness = (0.015625f) * multiplier;
                            log_drum_loudness = 0.5f + multiplier;
                        }
                    } else {
                        drum_loudness = (0.0078125f) * multiplier;
                        log_drum_loudness = multiplier;
                    }
                } else {
                    drum_loudness = (0.00390625f) * multiplier;
                }
            } else {
                drum_loudness = 0.0f;
            }
        }
        last_drum_position = drum_position;

#if 0
        if (sampleID >= MZK_NUMSAMPLES - 100000) {
            total_speed = 0.0f;
            if (sampleID == MZK_NUMSAMPLES - 100000) drum_loudness = 20.0f;
            else drum_loudness = 0.0f;
            has_ended = true;
        }
#endif

        if (drum_loudness > loudness) {
            loudness = drum_loudness;
        }

        //loudness += drum_loudness;
        cur_accumulated_drum_volume += log_drum_loudness * loudness;

        const float kAmplitudeReduction = 1.0f - 0.001f * (total_speed + 0.05f);
        //floatOutput[sample][0] = (total_speed + 0.02f) * 32.0f * loudness * lowNoise[sampleID % RANDOM_BUFFER_SIZE];
        floatOutput[sample][0] = 0.0f;
        //floatOutput[sample][1] = (total_speed + 0.02f) * 32.0f * loudness * lowNoise[(sampleID+800) % RANDOM_BUFFER_SIZE];;
        floatOutput[sample][1] = 0.0f;
        loudness *= kAmplitudeReduction;
        
        // And the sin:
        //const float kOvertoneMultiplier = 0.7f;
        base_frequency *= 1.0000015f;
        if (base_frequency > 2.0f * kMinBaseFrequency) {
            base_frequency -= kMinBaseFrequency;
        }
        float half_overtone_kinda_log_amount =
            (base_frequency - kMinBaseFrequency) / kMinBaseFrequency;
        // yeah, well, almost...
        float half_overtone_amount = half_overtone_kinda_log_amount * half_overtone_kinda_log_amount;
        base_offset += base_frequency / 44100.0f * PI * 2.0f;
        if (base_offset > PI * 2.0f) base_offset -= 2.0f * PI;
        //float overtone_loudness = 0.02f;
        float total = 0.0f;
        const float max_frequency = (4.0f * total_speed * total_speed + 0.01f) * 16000.0f + 2000.0f;
        for (int overtone = 1; overtone < 64; overtone++) {
            float frequency = overtone * base_frequency;
            float overtone_loudness = cosf(frequency / max_frequency * PI * 0.5f);
            if (frequency < max_frequency) {
                overtone_loudness *= overtone_loudness;
                overtone_loudness *= overtone_loudness * (total_speed);
                overtone_loudness *= 0.06f;
                float offset = overtone * base_offset;
                float overtone_multiplier = 1.0f;
                if (overtone & 1) overtone_multiplier = half_overtone_amount;
                total += sinf(offset) * overtone_loudness * overtone_multiplier;
                //overtone_loudness *= kOvertoneMultiplier;
            }
        }
        floatOutput[sample][0] += loudness * total * 4.0f;
        floatOutput[sample][1] += loudness * total * 4.0f;
        
        sampleID++;
    }

    startSampleID = sampleID;
    last_loudness = loudness;

	// Copy to int output
	for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
	{
#if 0
		int val = (int)floatOutput[0][sample];
		if (val > 32767) val = 32767;
		if (val < -32767) val = -32767;
		blockBuffer[sample] = val;
#else
		float val = floatOutput[0][sample];
		if (val > 1.0f) val = 1.0f;
		if (val < -1.0f) val = -1.0f;
		val = (float)sin(val) * 32512.0f;
		blockBuffer[sample] = ftoi_fast(val);
#endif
	}

    accumulated_drum_volume += cur_accumulated_drum_volume;

#ifdef WRITE_MUSIC
    FILE *fid = fopen("music.raw", "ab");
    fwrite(blockBuffer, sizeof(*blockBuffer), MZK_BLOCK_SIZE * 2, fid);
    fclose(fid);
#endif
}

// put here your synth
#pragma code_seg(".mzkInit")
void mzk_init()
{
	// Create data tables
	// make frequency (Hz) table
#if 0
	float k = 1.059463094359f;	// 12th root of 2
	float a = 6.875f;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (int i = 0; i < kNumFrequencies; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}
#endif

	// Make table with random number
	seed = 1;
	for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
	{
		randomBuffer[i] = frand();
		expRandomBuffer[i] = (float)exp2jo(LOG_2_E * 4.0f * (randomBuffer[i] - 1));
		lowNoise[i] = 4.0f * (randomBuffer[i] - 0.5f);
	}

#if 0
	// Set the oscillator phases to zero
	for (int inst = 0; inst < NUM_INSTRUMENTS; inst++)
	{
		for (int i = 0; i < NUM_OVERTONES; i++)
		{
			for (int j = 0; j < NUM_Stereo_VOICES; j++)
			{
				fPhase[inst][i][j] = 0.f;
			}
		}

		// Don't play instrument
		iADSR[inst] = 3;
	}
#endif

	// Ring-low-pass-filtering of lowPass
	// Use a one-pole
	float oldVal = 0.0f;
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
		{
			lowNoise[i] = 1.0f / 8.0f * lowNoise[i] + (1.0f - 1.0f / 8.0f) * oldVal;
			oldVal = lowNoise[i];
		}
	}

	// Clear reverberation buffer
#if 0
	for (int inst = 0; inst < NUM_INSTRUMENTS; inst++)
	{
		for (int i = 0; i < NUM_Stereo_VOICES; i++)
		{
			for (int j = 0; j < MAX_DELAY_LENGTH; j++)
			{
				reverbBuffer[inst][i][j] = 0;
			}
			reverbBufferLength[inst][i] = 1;
		}
	}
#endif


#if 0
	// create some test output
	for (int block = 0; block < MZK_DURATION; block++)
	{
		// 2x due to stereo
		short *blockBuffer = buffer + 2 * block*MZK_BLOCK_SIZE;
		mzkPlayBlock(blockBuffer);
	}
#endif

    // Warm up the output with one iteration
    for (int i = 0; i < 874; i++) {
        short block_buffer[MZK_BLOCK_SIZE * 2];
        mzkPlayBlock(block_buffer);
    }
}
