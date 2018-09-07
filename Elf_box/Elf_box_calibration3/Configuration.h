#pragma once

#define XRES 1024
#define YRES 768


#if 0  // Microsoft camera
// <--- Set to 0 to use default system webcam.
#define WEBCAM_DEVICE_INDEX 0

//#define MEDIA_TYPE_INDEX 29  /* 320x240, YUY2, 30 FPS, ~37MBPS */
#define MEDIA_TYPE_INDEX 0  /* 640x480, YUY2, 30 FPS, ~147MBPS */
//#define MEDIA_TYPE_INDEX 53  /* 1280x720, YUY2, 30 FPS, ~442MBPS */
#else  // Aukey webcam
// <--- Set to 0 to use default system webcam.
#define WEBCAM_DEVICE_INDEX 0

#define MEDIA_TYPE_INDEX 62 /* 640x480, YUY2, 30 FPS, ~147MBPS */
#endif

// Maximum distance where a pixel is considered nearby
#define CLOSE_PIXEL_DISTANCE 15

// Using multisample sounds like a bad idea
//#define USE_MULTISAMPLE

// Time until calibration starts after program launch in ms
//#define CALIBRATION_DELAY (240 * 1000)
#define CALIBRATION_DELAY (1 * 1000)
// Must be more than in latency measurement
//#define USE_BACKGROUND_UPDATE (2 * 1000)
//#define USE_LATENCY 1000
//#define NUM_ACCUMULATE 90
#define USE_BACKGROUND_UPDATE (500)
#define USE_LATENCY 400
#define NUM_ACCUMULATE 90

// Parameters to ignore larger fields
#define SUPPRESS_RELATIVE_BRIGHTNESS 0.5
#define SUPPRESS_RELATIVE_BRIGHTNESS_NEXT 0.1
#define SUPPRESS_MINIMUM_SIZE 10

#define CALIBRATION_X_RESOLUTION 1280
#define CALIBRATION_Z_RESOLUTION 800
//#define CALIBRATION_X_RESOLUTION 20
//#define CALIBRATION_Z_RESOLUTION 20

#define USE_CAMERA
//#define LIST_DEVICES
//#define LATENCY_MEASUREMENT
#define SHOW_HEIGHT
//#define MATRIX_CALIBRATION