#pragma once

#define XRES 640
#define YRES 480

// <--- Set to 0 to use default system webcam.
#define WEBCAM_DEVICE_INDEX 1
// 320x240, YUY2, 30 FPS, ~9MBPS
#define MEDIA_TYPE_INDEX 29

// In ms, must be more than in latency measurement
#define USE_LATENCY 300

#define CALIBRATION_X_RESOLUTION 50
#define CALIBRATION_Z_RESOLUTION 50

#define USE_CAMERA
//#define LIST_DEVICES
//#define LATENCY_MEASUREMENT
#define POINT_CALIBRATION