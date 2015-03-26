//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

// /CRINKLER /VERBOSE:FUNCTIONS /VERBOSE:IMPORTS /VERBOSE:LABELS /HASHTRIES:300 /COMPMODE:SLOW /ORDERTRIES:4000 

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define SETRESOLUTION
//#define CLEANEXIT
//#define XRES        (1280)
//#define YRES        (1080 * 1280 / 1920)
#define XRES        1000
#define YRES        (1080 * XRES / 1920)
#define OFFSCREEN_WIDTH 512
#define OFFSCREEN_HEIGHT 288

// Use midi to check parameter values
//#define EDIT_PARAMETERS
#define EFFECT_START_TIME 0.0f
#ifndef EDIT_PARAMETERS
//#define USEDSOUND
#endif

#endif
