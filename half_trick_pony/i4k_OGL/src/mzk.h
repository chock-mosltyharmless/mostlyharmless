//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#ifndef _MZK_H_
#define _MZK_H_

#define MZK_BLOCK_SIZE  4096
#define MZK_DURATION    1292
#define MZK_RATE        44100
#define MZK_NUMCHANNELS 2

#define MZK_NUMSAMPLES  (MZK_DURATION*MZK_BLOCK_SIZE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES*MZK_NUMCHANNELS)

void mzk_init( short *buffer );


#endif