//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#ifndef _MZK_H_
#define _MZK_H_

// I dislike MZK_DURATION???
#define MZK_DURATION    163
#define MZK_RATE        44100
#define MZK_NUMCHANNELS 2
// Audio buffer size in samples
#define AUDIO_BUFFER_SIZE 8192

#define MZK_NUMSAMPLES  (MZK_DURATION*MZK_RATE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES*MZK_NUMCHANNELS)

void mzk_init(void);
// Call this function to prepare the next audio buffer.
// The audio buffer size is AUDIO_BUFFER_SIZE
void mzk_prepare_block(short *buffer);
float frand(unsigned int *seed);

#endif