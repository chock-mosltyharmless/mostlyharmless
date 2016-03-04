#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once

#define PIF 3.1415926f

// This header contains all the configuration macros that are
// used to set the resolution and so on.
#define XRES 800
#define YRES 600

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


#endif