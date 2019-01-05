#pragma once

#define XRES 1024
#define YRES 768

// <--- Set to 0 to use default system webcam.
#define WEBCAM_DEVICE_INDEX 0

//#define MEDIA_TYPE_INDEX 29  /* 320x240, YUY2, 30 FPS, ~37MBPS */
#define MEDIA_TYPE_INDEX 0  /* 640x480, YUY2, 30 FPS, ~147MBPS */
//#define MEDIA_TYPE_INDEX 53  /* 1280x720, YUY2, 30 FPS, ~442MBPS */
// Maximum distance where a pixel is considered nearby
#define CLOSE_PIXEL_DISTANCE 15

// Time until calibration starts after program launch in ms
#define CALIBRATION_DELAY (120 * 1000)
// Must be more than in latency measurement
#define USE_BACKGROUND_UPDATE (2 * 1000)
#define USE_LATENCY 1000
#define NUM_ACCUMULATE 90

#define CALIBRATION_X_RESOLUTION 600
#define CALIBRATION_Z_RESOLUTION 600

#define USE_CAMERA
//#define LIST_DEVICES
//#define LATENCY_MEASUREMENT
//#define POINT_CALIBRATION  DEFUNCT!
//#define SHOW_HEIGHT
#define MATRIX_CALIBRATION