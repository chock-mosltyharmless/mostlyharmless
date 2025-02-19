#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once

// This header contains all the configuration macros that are
// used to set the resolution and so on.
#define XRES 1280
#define YRES (XRES*9/16)

// The resolution of the background stuff
#define X_OFFSCREEN XRES
#define Y_OFFSCREEN YRES
#define X_HIGHLIGHT 320
#define Y_HIGHLIGHT 180

// The maximum number of chars in an error message.
// Make error fields one larger!
#define MAX_ERROR_LENGTH (32*1024)

// Play back music during coding
#define MUSIC

#endif