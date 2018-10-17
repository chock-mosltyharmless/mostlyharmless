#pragma once

#define XRES 1024
#define YRES 768

// <--- Set to 0 to use default system webcam.
#define WEBCAM_DEVICE_INDEX 0

//#define MEDIA_TYPE_INDEX 29  /* 320x240, YUY2, 30 FPS, ~37MBPS */
#define MEDIA_TYPE_INDEX 0  /* 640x480, YUY2, 30 FPS, ~147MBPS */
//#define MEDIA_TYPE_INDEX 53  /* 1280x720, YUY2, 30 FPS, ~442MBPS */

// Using multisample sounds like a bad idea
//#define USE_MULTISAMPLE

// In ms
#define CALIBRATION_DELAY (20 * 1000)
// Must be more than in latency measurement
#define USE_BACKGROUND_UPDATE (2 * 1000)
#define USE_LATENCY 500
#define NUM_ACCUMULATE 50

#define CALIBRATION_X_RESOLUTION 1280
#define CALIBRATION_Z_RESOLUTION 800

//#define SHOW_HEIGHT
//#define SHOW_BALL
#define SHOW_RAINBOW_BALL