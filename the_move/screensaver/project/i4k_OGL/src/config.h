//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

// /CRINKLER /VERBOSE:FUNCTIONS /VERBOSE:IMPORTS /VERBOSE:LABELS /HASHTRIES:300 /COMPMODE:SLOW /ORDERTRIES:4000 

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define SETRESOLUTION
//#define USEDSOUND
//#define CLEANEXIT
//#define XRES        (1280)
//#define YRES        (1080 * 1280 / 1920)
#define XRES        1200
#define YRES        (1080 * XRES / 1920)
//#define YRES        (480 * XRES / 640)
#define ASPECT_RATIO ((float)XRES / (float)YRES)
#define OFFSCREEN_WIDTH XRES
#define OFFSCREEN_HEIGHT YRES
#define HIGHLIGHT_WIDTH 200
#define HIGHLIGHT_HEIGHT 100
//#define MZK_DURATION 123
#define MZK_DURATION 451

#define MAX_ERROR_LENGTH 4096

#endif
