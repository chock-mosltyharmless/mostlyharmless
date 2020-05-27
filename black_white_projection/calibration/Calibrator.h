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
    bool Calibrate();

    // This is a mode that tries to create one solid color for the whole screen (except borders)
    void ShowConstColor(unsigned char red, unsigned char green, unsigned char blue);

private:
    void DrawCameraDebug(HDC debug_window_device_handle, HGLRC debug_window_resource_handle);

    Camera camera_;

    // This is a mapping from camera coordinates to projector virtual coordinates
    // in the projection texture
    int(*camera_to_projector_)[2];

    // This is the GPU representation of the projector texture
    GLuint texture_id_;

    // This RGBA buffer holds the CPU data for what is shown in the texture
    unsigned char (*project_buffer_)[4];

    // Data for where to display stuff
    HDC main_window_device_handle_;
    HGLRC main_window_resource_handle_;
    HDC debug_window_device_handle_;
    HGLRC debug_window_resource_handle_;
};

