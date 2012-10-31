//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

// /CRINKLER /VERBOSE:FUNCTIONS /VERBOSE:IMPORTS /VERBOSE:LABELS /HASHTRIES:300 /COMPMODE:SLOW /ORDERTRIES:4000 

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define NO_HERMANN_LIGHTING
//#define SETRESOLUTION
//#define USEDSOUND
//#define CLEANEXIT
//#define XRES        (1280)
//#define YRES        (1080 * 1280 / 1920)
#ifndef NO_HERMANN_LIGHTING
#define XRES        1900
#define YRES        (480 * XRES / (640 * 2))
#else
#define XRES        1024
#define YRES        (1080 * XRES / 1920)
#endif
#define OFFSCREEN_WIDTH 1024
#define OFFSCREEN_HEIGHT (1080 * OFFSCREEN_WIDTH / 1920)
#define HIGHLIGHT_WIDTH 400
#define HIGHLIGHT_HEIGHT (480 * HIGHLIGHT_WIDTH / 640)

#endif