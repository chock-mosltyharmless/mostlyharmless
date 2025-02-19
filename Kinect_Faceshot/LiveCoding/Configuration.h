#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once

// This header contains all the configuration macros that are
// used to set the resolution and so on.
#define XRES 1024
#define YRES (1024*9/16)

// The resolution of the background stuff
#define X_OFFSCREEN XRES
#define Y_OFFSCREEN YRES
#define X_HIGHLIGHT 400
#define Y_HIGHLIGHT 300
#define TGA_WIDTH (512 * 8)
#define TGA_HEIGHT (424 * 8)

// The maximum number of chars in an error message.
// Make error fields one larger!
#define MAX_ERROR_LENGTH (32*1024)

// Play back music during coding
//#define MUSIC

#endif