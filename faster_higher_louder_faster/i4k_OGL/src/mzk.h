//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#ifndef _MZK_H_
#define _MZK_H_

#define MZK_BLOCK_SIZE  512
#define AUDIO_BUFFER_SIZE MZK_BLOCK_SIZE
#define MZK_DURATION    (1292*8)
#define MZK_RATE        44100
#define MZK_NUMCHANNELS 2

#define MZK_NUMSAMPLES  (MZK_DURATION*MZK_BLOCK_SIZE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES*MZK_NUMCHANNELS)

// Debug: Dump music to file
//#define WRITE_MUSIC

void mzk_init();
void mzkPlayBlock(short *blockBuffer);


#endif