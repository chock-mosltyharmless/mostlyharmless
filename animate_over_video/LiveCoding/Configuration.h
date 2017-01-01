#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once

// This header contains all the configuration macros that are
// used to set the resolution and so on.
#define XRES 800
#define YRES 600

// The resolution of the background stuff
#define X_OFFSCREEN 400
#define Y_OFFSCREEN 300
#define X_HIGHLIGHT 128
#define Y_HIGHLIGHT 64

// The maximum number of chars in an error message.
// Make error fields one larger!
#define MAX_ERROR_LENGTH (32*1024)


#endif