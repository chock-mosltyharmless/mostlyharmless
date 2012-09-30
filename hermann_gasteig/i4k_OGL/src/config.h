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
#define XRES        1800
#define YRES        (480 * XRES / (640 * 2))
#define OFFSCREEN_WIDTH 900
#define OFFSCREEN_HEIGHT (480 * OFFSCREEN_WIDTH / 640)
#define HIGHLIGHT_WIDTH 400
#define HIGHLIGHT_HEIGHT (480 * HIGHLIGHT_WIDTH / 640)

#endif