//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#ifndef _INTRO_H_
#define _INTRO_H_

void compileShaders(void);
void intro_init(void);
void intro_do(long time);
void intro_end(void);
void intro_left_click(float xpos, float ypos, int itime, int sound);
void intro_right_click(float xpos, float ypos, int itime, int sound);
void intro_cursor(float xpos, float ypos);
void intro_blackout(bool becomesBlack);

extern HWND hWnd;

#endif
