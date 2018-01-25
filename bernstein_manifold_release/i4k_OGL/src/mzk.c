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
#define LOG_2_E 1.44269f
#define fScaler ((float)((double)2*PI / (double)MZK_RATE))

#define NUM_INSTRUMENTS 18
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

static float randomBuffer[RANDOM_BUFFER_SIZE];
static float lowNoise[RANDOM_BUFFER_SIZE];
// The same as random Buffer, but contains exp(4*(randomBuffer-1))
//static float expRandomBuffer[RANDOM_BUFFER_SIZE];
#if 0
static float reverbBuffer[NUM_INSTRUMENTS][MAX_DELAY_LENGTH][NUM_Stereo_VOICES];
static int reverbBufferLength[NUM_Stereo_VOICES]; // Actual length taken for pull-out
                                                  // The note index that the instrument is on
#endif
static int currentNoteIndex[NUM_INSTRUMENTS];
static int currentNote[NUM_INSTRUMENTS];
// TODO: One phase is enough
static float fPhase[NUM_INSTRUMENTS]; // Phase of the instrument
                                                     //static float fTimePoint[NUM_INSTRUMENTS];
                                                     //static float fModulationPhase[NUM_INSTRUMENTS];

                                                     // From vstxsynthproc.cpp
//static float fShape[NUM_INSTRUMENTS][4][NUM_OVERTONES];  // Overtone shape for each ADSR thingies
//static float detuneFactor[NUM_OVERTONES];  // Independent of instrument

                                    // Interpolated parameters
                                    // TODO: Make it one array
#if 0
float adsrVolume[NUM_INSTRUMENTS];
float adsrQuak[NUM_INSTRUMENTS];
float adsrDistort[NUM_INSTRUMENTS];
float adsrNoise[NUM_INSTRUMENTS];
float adsrDetune[NUM_INSTRUMENTS];
float adsrFreq[NUM_INSTRUMENTS];
float adsrSpeed[NUM_INSTRUMENTS];
float adsrShape[NUM_INSTRUMENTS][NUM_OVERTONES];
#else
#define NUM_ADSR_DATA 5
static float adsrData[NUM_INSTRUMENTS][NUM_ADSR_DATA];
#define adsrVolume 0
#define adsrQuak 1
#define adsrDistort 2
#define adsrNoise 3
#define adsrDetune -1
#define adsrFreq 4
// Shape has also overtones
#define adsrShape 6
#endif

// MIDI volume
static int i_midi_volume_[NUM_INSTRUMENTS];
//static float midi_volume_[NUM_INSTRUMENTS];

// attack values and stuff
static int iADSR[NUM_INSTRUMENTS];
static float fADSRVal[NUM_INSTRUMENTS];

static unsigned int seed;

// create random value between -65534 and 65534?
#if 0
#pragma code_seg(".jo_rand")
int jo_rand(unsigned int *s)
{
    unsigned long a = 214013;
    unsigned long c = 2531011;
    unsigned long m = 4294967296-1;
    *s = (*s * a + c) % m;
    return (*s >> 8) % 65535;
}
#endif

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

#if 0
#pragma code_seg(".ftoi_fast")
int ftoi_fast(float f)
{
    return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}
#endif

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
    static int startSampleID = 0;
    int sampleID;

    //set_mxcsr_on(FTZ_BIT);
    //set_mxcsr_off(UNDERFLOW_EXCEPTION_MASK);

    // clear audio block
    for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
    {
        // Stereo! --> 2 values
        floatOutput[0][sample] = 0;
    }

    //int on[18] = {0,0,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1};

    for (int instrument = 0; instrument < NUM_INSTRUMENTS; instrument++)
    {
        //if (!on[instrument])continue;
        // Everything * 16 as only every 16th sample it's done.
        //const int kADSRSuperSample = 1;
        //const float kSuperSample = (float)kADSRSuperSample;

        // Go over all samples
        sampleID = startSampleID;

        // Volume?
        // Get parameters locally
        float vol = float_instrument_parameters_[instrument][F_MASTER_VOLUME];
        float panning = float_instrument_parameters_[instrument][K_MASTER_PANNING];
        //float invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f * kSuperSample;
        float invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f;

#if 0
        // Set the reverberation buffer length at key start (no interpolation)
        reverbBufferLength[0] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR + 1;
        reverbBufferLength[1] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 7 / 17 + 1;
        reverbBufferLength[2] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 13 / 23 + 1;
        reverbBufferLength[3] = instrumentParams[instrument][K_DELAY_LENGTH] * DELAY_MULTIPLICATOR * 11 / 13 + 1;
#endif

        // Check if we go to next note
        if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0)
        {
            if (savedNote[instrument][currentNoteIndex[instrument]] != -128)
            {
                // Key on
                iADSR[instrument] = 0; // attack
                fADSRVal[instrument] = 0.0f; // starting up

                for (int i = 0; i < NUM_ADSR_DATA; i++) {
                    adsrData[instrument][i] = float_instrument_parameters_[instrument][i * 4 + iADSR[instrument]];
                }
                //for (int i = 0; i < NUM_OVERTONES; i++) {
                //    adsrData[instrument][adsrShape + i] = fShape[instrument][iADSR[instrument]][i];
                //}

                // Apply delta-note
                currentNote[instrument] += savedNote[instrument][currentNoteIndex[instrument]];
                i_midi_volume_[instrument] += savedVelocity[instrument][currentNoteIndex[instrument]];
                //midi_volume_[instrument] = i_midi_volume_[instrument] / 128.0f;
                //midi_volume_[instrument] = 1.0f;

                // Set the oscillator phases to zero
                fPhase[instrument] = 0.f;
            }
            else
            {
                // NoteOff
                iADSR[instrument] = 2; // Release
            }

            //invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f * kSuperSample;
            invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f;

            // Go to next note location
            currentNoteIndex[instrument]++;
        }
        savedNoteTime[instrument][currentNoteIndex[instrument]]--;

        // ignore everything before the first note
        if (currentNoteIndex[instrument] == 0) {
            continue;
        }

        if (float_instrument_parameters_[instrument][K_VOLUME + iADSR[instrument] + 1] < 1e-4 && adsrData[instrument][adsrVolume] < 1e-4) continue;

        // Get audio frequency
        //float baseFreq = freqtab[currentNote[instrument] & 0x7f] * fScaler;
        float baseFreq = 8.175f * (float)exp2jo((float)currentNote[instrument] * (1.0f/12.0f)) * fScaler;

        //for (int super_sample = 0; super_sample < MZK_BLOCK_SIZE; super_sample += kADSRSuperSample)
#define super_sample sample
        for (int sample = 0; sample < MZK_BLOCK_SIZE; sample++)
        {
            float deathVolume = 1.0f;

            // Process ADSR envelope
            if (iADSR[instrument] == 0) {
                fADSRVal[instrument] += (1.0f - fADSRVal[instrument]) *
                    //(float_instrument_parameters_[instrument][K_ADSR_SPEED + 0] / 1024.0f) * kSuperSample;
                    (float_instrument_parameters_[instrument][K_ADSR_SPEED + 0] / 1024.0f);
            }
            // Go from attack to decay
            if (iADSR[instrument] == 0 && fADSRVal[instrument] > 0.75) {
                iADSR[instrument] = 1;
                //invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f * kSuperSample;
                invADSRSpeed = float_instrument_parameters_[instrument][K_ADSR_SPEED + iADSR[instrument]] / 1024.0f;
            }

            // interpolate volume according to ADSR envelope
            for (int i = 0; i < NUM_ADSR_DATA; i++) {
                adsrData[instrument][i] += (float_instrument_parameters_[instrument][i*4 + iADSR[instrument] + 1] - adsrData[instrument][i]) * invADSRSpeed;
                if (adsrData[instrument][i] < 1e-6f) adsrData[instrument][i] = 0.0f;
            }
            //for (int i = 0; i < NUM_OVERTONES; i++) {
                //adsrData[instrument][adsrShape + i] += (fShape[instrument][iADSR[instrument] + 1][i] - adsrData[instrument][adsrShape + i]) * invADSRSpeed;
            //}

            // fade out on new instrument (but not on noteoff...)
            // (deathcounter)
            if (savedNoteTime[instrument][currentNoteIndex[instrument]] == 0 &&
                super_sample >= MZK_BLOCK_SIZE - 512 &&
                savedNote[instrument][currentNoteIndex[instrument]] != -128)
            {
                deathVolume = (MZK_BLOCK_SIZE - super_sample) / 512.0f;
            }

            float base_phase = fPhase[instrument];
            //for (int sample = super_sample; sample < super_sample + kADSRSuperSample; sample++) {            
            {
                // Some optimziation?
                //if (fADSRVal[instrument] < 1.0f / 65536.0f) fADSRVal[instrument] = 0.0f;

                float outAmplitude[2] = {0};

#if 1
                //int maxOvertones = (int)(3.0f / baseFreq);
                float overtoneLoudness = 1.0f;
                //float overallLoudness = 1.0f / 65536.0f;
                //if (adsrData[instrument][adsrVolume] > 1e-4 || float_instrument_parameters_[instrument][K_VOLUME + iADSR[instrument] + 1] > 1e-4)
                    float phase = base_phase;
                    for (int i = 0; i < NUM_OVERTONES; i++)
                    {
                        //if (i < maxOvertones)
                        {
                            outAmplitude[0] += sinf(phase) * overtoneLoudness
                                /** adsrData[instrument][adsrShape+i]*/ * (1.0f - panning);
                            outAmplitude[1] += sinf(phase) * overtoneLoudness
                                /** adsrData[instrument][adsrShape+i]*/ * panning;
#if 0
                            fPhase[instrument][i] += baseFreq * (i+1) * (adsrData[instrument][adsrFreq]*4.0f);
                                //* (1.0f + adsrData[instrument][adsrDetune] * detuneFactor[i] * 0.5f);
                            while (fPhase[instrument][i] > 2.0f * PI) fPhase[instrument][i] -= 2.0f * (float)PI;
#else
                                phase += base_phase;
#endif
                        }
                        //overallLoudness += overtoneLoudness /** adsrData[instrument][adsrShape+i]*/;
                        overtoneLoudness *= adsrData[instrument][adsrQuak] * 2.0f;
                    }

                    base_phase += baseFreq * (adsrData[instrument][adsrFreq] * 4.0f);
                    while (base_phase > 2.0f * PI) base_phase -= 2.0f * (float)PI;
#else
                float overallLoudness = 1.0f;
                outAmplitude[0] += (float)sin(fPhase[instrument][0]);
                fPhase[instrument][0] += baseFreq;
                while (fPhase[instrument][0] > 2.0f * PI) fPhase[instrument][0] -= 2.0f * (float)PI;
#endif

#if 1
                // Ring modulation with noise
                float cur_vol = vol * adsrData[instrument][adsrVolume] * deathVolume * i_midi_volume_[instrument] / 128.0f;//(midi_volume_[instrument];
                float distortMult = (float)exp2jo(LOG_2_E * 6.0f*adsrData[instrument][adsrDistort]);
                for (int i = 0; i < 2; i++)
                {
                    float output = outAmplitude[i];
                    // Ring modulation with noise
                    output *= 1.0f + (lowNoise[sampleID % RANDOM_BUFFER_SIZE] - 1.0f) * adsrData[instrument][adsrNoise];

                    // Distort
                    output *= distortMult;
#if 1
                    output = 2.0f * (1.0f / (1.0f + (float)exp2jo(LOG_2_E * -output)) - 0.5f);
#else

                    if (output > 1.5f) output = 1.5f;
                    if (output < -1.5f) output = -1.5f;
                    output = (float)sinf(output);
#endif
                    output /= distortMult;
                    floatOutput[sample][i] += output * cur_vol;
                }
#endif

#if DO_REVERB
                // Put everything into the reverb buffers
                int reverbPos = sampleID % MAX_DELAY_LENGTH;
                for (int j = 0; j < NUM_Stereo_VOICES; j++)
                {
                    reverbBuffer[instrument][reverbPos][j] = outAmplitude[j] * vol * adsrData[instrument][adsrVolume] * deathVolume * midi_volume_[instrument];

                    // Do the reverb feedback
                    int fromBuffer = (j + 1) % NUM_Stereo_VOICES;
                    int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[fromBuffer]);
                    reverbBuffer[instrument][reverbPos][j] += float_instrument_parameters_[instrument][K_DELAY_FEED] * reverbBuffer[instrument][fromLocation % MAX_DELAY_LENGTH][fromBuffer];
                    if (fabsf(reverbBuffer[instrument][reverbPos][j]) < 1.0e-12) reverbBuffer[instrument][reverbPos][j] = 0.0f;
                }
#endif

#if 0
                // Put everything into the reverb buffers
                int reverbPos = sampleID % MAX_DELAY_LENGTH;
                float totalLoudness = 0.5f;

                // Adjust volume
                float ampl = outAmplitude[0];

                reverbBuffer[instrument][reverbPos][0] = totalLoudness * ampl;
#endif

                // Do the reverb feedback
#if 0
                int fromBuffer = (j + 1) % NUM_Stereo_VOICES;
                int fromLocation = (reverbPos + MAX_DELAY_LENGTH - reverbBufferLength[fromBuffer]) % 
                    MAX_DELAY_LENGTH;
                reverbBuffer[instrument][reverbPos][j] += fDelayFeed * reverbBuffer[instrument][fromLocation][fromBuffer];
#endif
                // Ignore denormals..
                //if (fabsf(reverbBuffer[instrument][reverbPos][j]) < 1.0e-12) reverbBuffer[instrument][reverbPos][j] = 0.0f;

#if 0
                floatOutput[sample][0] += reverbBuffer[instrument][reverbPos][0];
                floatOutput[sample][0] += reverbBuffer[instrument][reverbPos][2];
                floatOutput[sample][1] += reverbBuffer[instrument][reverbPos][1];
                floatOutput[sample][1] += reverbBuffer[instrument][reverbPos][3];
#endif
                sampleID++;
            }
            fPhase[instrument] = base_phase;
        }
    }

    // Copy to int output
    for (int sample = 0; sample < MZK_BLOCK_SIZE * 2; sample++)
    {
#if 0
        int val = (int)floatOutput[0][sample];
        if (val > 32767) val = 32767;
        if (val < -32767) val = -32767;
        blockBuffer[sample] = val;
#else
        float val = floatOutput[0][sample] * 8.0f;
#if 0
        if (val > 1.5f) val = 1.5f;
        if (val < -1.5f) val = -1.5f;
        val = (float)sinf(val) * 32768.0f;
#else
        val = 2.0f * 32768.0f * (1.0f / (1.0f + (float)exp2jo(LOG_2_E * -val)) - 0.5f);
#endif
        blockBuffer[sample] = _mm_cvtt_ss2si(_mm_load_ss(&val));
#endif
    }

    startSampleID = sampleID;
}

// put here your synth
#pragma code_seg(".mzk_init")
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
        randomBuffer[i] = jo_frand(&seed);
        //expRandomBuffer[i] = (float)exp2jo(LOG_2_E * 4.0f * (randomBuffer[i] - 1));
        lowNoise[i] = 16.0f * (randomBuffer[i] - 0.5f);
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
    for (int j = 0; j < 8; j++)
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

    // Convert int parameters to float parameters
    for (int inst = 0; inst < NUM_INSTRUMENTS; inst++) {
        for (int param = 0; param < NUM_INSTRUMENT_PARAMETERS; param++) {
            float_instrument_parameters_[inst][param] = (float)instrumentParams[inst][param] / 128.0f;
        }
    }

#if 0
    // Set the overtone shape of the oscilators for the 4 different ADSR stuffsies
    // Some intermediate setting of shape stuff
    for (int inst = 0; inst < NUM_INSTRUMENTS; inst++) {
        for (int i = 0; i < NUM_OVERTONES; i++) {
            for (int j = 0; j < 4; j++) {
                fShape[inst][j][i] = expRandomBuffer[i + instrumentParams[inst][K_SHAPE + j]*NUM_OVERTONES];
            }
        }
    }
#endif

#if 0
    // Detuning constants
    for (int i = 0; i < NUM_OVERTONES; i++) {
        detuneFactor[i] = (randomBuffer[i] - 0.5f);
    }
#endif
}
