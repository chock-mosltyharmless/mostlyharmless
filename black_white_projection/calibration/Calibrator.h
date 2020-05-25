#pragma once

#include "Camera.h"

// This class creates and holds calibration data
// It also holds the current render state and can be used to go to destination state
class Calibrator
{
public:
    Calibrator();
    virtual ~Calibrator();

    bool Init(HDC main_window_device_handle, HGLRC main_window_resource_handle,
              HDC debug_window_device_handle, HGLRC debug_window_resource_handle);
    bool Calibrate(HDC main_window_device_handle, HGLRC main_window_resource_handle,
                   HDC debug_window_device_handle, HGLRC debug_window_resource_handle);

private:
    void DrawCameraDebug(HDC debug_window_device_handle, HGLRC debug_window_resource_handle);

    Camera camera_;
    int(*camera_to_projector_)[2];
};

