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
static int currentNoteIndex[NUM_INSTRUMENTS];
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

// TODO: Check implementation from somewhere else. Esp. %65535? Numeric recipies.
#pragma code_seg(".jo_frand")
float jo_frand(unsigned int *s)
{
    unsigned long a = 214013;
    unsigned long c = 2531011;
    unsigned long m = 4294967296-1;
    *s = (*s * a + c) % m;
    //return (*s >> 8) % 65535;
    return (float)((*s >> 8) % 65535) * (1.0f/65536.0f);
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

#pragma code_seg(".mzkPlayBlock")
static float floatOutput[MZK_BLOCK_SIZE][2];
void mzk_prepare_block(short *blockBuffer)
{
    // clear audio block
    for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
    {
        // Stereo! --> 2 values
        floatOutput[0][sample] = 0;
    }

    //int on[18] = {0,0,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1};

    for (int instrument = 0; instrument < NUM_INSTRUMENTS; instrument++)
    {
        int note_pos = savedNotePos[instrument];
        //if (!on[instrument])continue;

        // Volume?
        // Get parameters locally
        float vol = float_instrument_parameters_[instrument][F_MASTER_VOLUME];
        float panning = float_instrument_parameters_[instrument][K_MASTER_PANNING];
        float invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);

        // Check if we go to next note
        if (savedNoteTime__[note_pos + currentNoteIndex[instrument]] == 0) {
            if (savedNote__[note_pos + currentNoteIndex[instrument]] != -128) {
                // Key on
                iADSR[instrument] = 0; // attack
                fADSRVal[instrument] = 0.0f; // starting up

                for (int i = 0; i < NUM_ADSR_DATA; i++) {
                    adsrData[instrument][i] = float_instrument_parameters_[instrument][i * 4 + iADSR[instrument]];
                }

                // Apply delta-note
                currentNote[instrument] += savedNote__[note_pos + currentNoteIndex[instrument]];
                i_midi_volume_[instrument] += savedVelocity__[currentNoteIndex[instrument] + velocityPos[instrument]];

                // Set the oscillator phases to zero
                fPhase[instrument] = 0.f;
            } else {
                // NoteOff
                iADSR[instrument] = 2; // Release
            }

            invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);

            // Go to next note location
            currentNoteIndex[instrument]++;
        }
        savedNoteTime__[note_pos + currentNoteIndex[instrument]]--;

        // ignore everything before the first note
        if (currentNoteIndex[instrument] == 0) {
            continue;
        }

        if (float_instrument_parameters_[instrument][K_VOLUME + iADSR[instrument] + 1] < (1.0f / 1024.0f) &&
            adsrData[instrument][adsrVolume] < (1.0f / 1024.0f)) continue;

        // Get audio frequency
        float baseFreq = 8.175f * (float)exp2jo((float)currentNote[instrument] * (1.0f/12.0f)) * fScaler;

        float base_phase = fPhase[instrument];
        for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++) {
            float deathVolume = 1.0f;

            // Process ADSR envelope
            if (iADSR[instrument] == 0) {
                fADSRVal[instrument] += (1.0f - fADSRVal[instrument]) *
                    (float_instrument_parameters_[instrument][K_ADSR_SPEED + 0] * (1.0f / 1024.0f));
            }
            // Go from attack to decay
            if (iADSR[instrument] == 0 && fADSRVal[instrument] > 0.75f) {
                iADSR[instrument] = 1;
                invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] * (1.0f / 1024.0f);
            }

            // interpolate volume according to ADSR envelope
            for (int i = 0; i < NUM_ADSR_DATA; i++) {
                adsrData[instrument][i] += (float_instrument_parameters_[instrument][i*4 + iADSR[instrument] + 1] - adsrData[instrument][i]) * invADSRSpeed;
            }

            if (savedNoteTime__[note_pos + currentNoteIndex[instrument]] == 0 &&
                sample >= MZK_BLOCK_SIZE - 1024 &&
                savedNote__[note_pos + currentNoteIndex[instrument]] != -128) {
                deathVolume = (MZK_BLOCK_SIZE - sample) * (1.0f / 1024.0f);
            }

            float outAmplitude = 0;

            float overtoneLoudness = 1.0f;
            float phase = base_phase;
            float overtone_falloff = adsrData[instrument][adsrQuak] * 2.0f;
            for (int i = 0; i < NUM_OVERTONES; i++) {
                outAmplitude += sinf(phase) * overtoneLoudness;
                phase += base_phase;
                // Ring modulation with noise
                outAmplitude *= 1.0f + (lowNoise[sample % RANDOM_BUFFER_SIZE] - 1.0f) * adsrData[instrument][adsrNoise];
                overtoneLoudness *= overtone_falloff;
            }

            base_phase += baseFreq * (adsrData[instrument][adsrFreq] * 4.0f);
            while (base_phase > 2.0f * PI) base_phase -= 2.0f * (float)PI;

            // Ring modulation with noise
            float cur_vol = vol * adsrData[instrument][adsrVolume] * deathVolume * i_midi_volume_[instrument] * (1.0f / 128.0f);
            float distortMult = (float)exp2jo(8.0f*adsrData[instrument][adsrDistort]);
            float current_pan = panning;
            for (int i = 0; i < 2; i++) {
                float output = outAmplitude * current_pan;

                // Distort
                output *= distortMult;
                output = 2.0f * (1.0f / (1.0f + (float)exp2jo(-2.0f * output)) - 0.5f);
                output /= distortMult;
                floatOutput[sample][i] += output * cur_vol;

                // Apply stereo
                current_pan = 1.0f - current_pan;
            }
        }
        fPhase[instrument] = base_phase;
    }

    // Copy to int output
    for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
    {
        float val = -8.0f * floatOutput[0][sample];
        val = 2.0f * 32768.0f * (1.0f / (1.0f + (float)exp2jo(val)) - 0.5f);
        blockBuffer[sample] = _mm_cvtt_ss2si(_mm_load_ss(&val));
    }
}

// put here your synth
#pragma code_seg(".mzk_init")
void mzk_init()
{
    // Create data tables

    // Make table with random number
    unsigned int seed = 1;
    for (int i = 0; i < RANDOM_BUFFER_SIZE; i++) {
        lowNoise[i] = 16.0f * (jo_frand(&seed) - 0.5f);
    }

    // Ring-low-pass-filtering of lowPass
    // Use a one-pole
    float oldVal = 0.0f;
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < RANDOM_BUFFER_SIZE; i++) {
            lowNoise[i] = 1.0f / 8.0f * lowNoise[i] + (1.0f - (1.0f / 8.0f)) * oldVal;
            oldVal = lowNoise[i];
        }
    }

    // Convert int parameters to float parameters
    for (int inst = 0; inst < NUM_INSTRUMENTS; inst++) {
        for (int param = 0; param < NUM_INSTRUMENT_PARAMETERS; param++) {
            float_instrument_parameters_[inst][param] = (float)instrumentParams[inst][param] * (1.0f / 128.0f);
        }
        
        // Delta-uncompress note locations
        savedNotePos[inst + 1] += savedNotePos[inst];
    }
}
