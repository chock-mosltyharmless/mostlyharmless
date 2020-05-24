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
// Use this for "AUKEY PC-LM1A Webcam"
#define MEDIA_TYPE_INDEX 62 /* 640x480, YUY2, 30 FPS, ~147MBPS */
#endif

// Maximum distance where a pixel is considered nearby
#define CLOSE_PIXEL_DISTANCE 20

// Using multisample sounds like a bad idea
//#define USE_MULTISAMPLE

// Time until calibration starts after program launch in ms
//#define CALIBRATION_DELAY (240 * 1000)
#define CALIBRATION_DELAY (5 * 1000)
// Must be more than in latency measurement
//#define USE_LATENCY 1000
//#define NUM_ACCUMULATE 90
#define USE_LATENCY 500
#define NUM_ACCUMULATE 10

// Resolution of the calibration
#define CALIBRATION_LOG_X_RESOLUTION 8     // width 256
#define CALIBRATION_X_RESOLUTION (1 << CALIBRATION_LOG_X_RESOLUTION)
#define CALIBRATION_LOG_Y_RESOLUTION 7     // height 128
#define CALIBRATION_Y_RESOLUTION (1 << CALIBRATION_LOG_Y_RESOLUTION)

#define CALIBRATION_THRESHOLD 80  // How bright the image has to be to be considered white.

//#define LIST_DEVICES
//#define LATENCY_MEASUREMENT
