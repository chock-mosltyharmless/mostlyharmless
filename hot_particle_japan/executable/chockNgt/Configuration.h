#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once

#define PIF 3.1415926f

// This header contains all the configuration macros that are
// used to set the resolution and so on.
#define XRES 1024
#define YRES 768

//#define ASPECT_RATIO (((float)XRES) / ((float)YRES))
#define ASPECT_RATIO (4.0f / 3.0f)

// The resolution of the background stuff
//#define X_OFFSCREEN 512
//#define Y_OFFSCREEN 256
extern int X_OFFSCREEN;
extern int Y_OFFSCREEN;
#define X_HIGHLIGHT 128
#define Y_HIGHLIGHT 64

// The maximum number of chars in an error message.
// Make error fields one larger!
#define MAX_ERROR_LENGTH (32*1024)

#define NO_SCENE_BRIGHTNESS 0.3f

//#define FULLSCREEN

#endif