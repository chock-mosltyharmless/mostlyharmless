#pragma once

#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
//#include <wmcodecdsp.h>

class Camera
{
public:
    Camera();
    virtual ~Camera();

    int Init(void);

    // This method  is used to calibrate the camera to black before the light goes on.
    void CalibrateCameraToBlack(void) {
        reference_black_brightness_ = GetAverageBrightness();
    }

    // Use this to calibrate what white means
    void CalibrateCameraToWhite(void)
    {
        reference_white_brightness_ = GetAverageBrightness();
    }

    // Called to get and accumulate next frame of camera.
    int Camera::GetFrame(bool refresh_accumulate);

    // Must be Called after all the data was accumulated  to retrieve accumulate buffer
    void NormalizeAccumulateBuffer(void);

    // Detect whether the camera saw light in the last Update()
    bool DetectFlash();

    // Set texture of captured image
    void SetTexture(bool show_raw_signal = false);

    int width() const { return width_; }
    int height() const { return height_; }

    int *accumulate_buffer_;  // RGBA sum of values over time, if no background update is done
    unsigned char raw_data(int index) {
        return buffer_[index];
    }

private:
    float GetAverageBrightness(void);  // In accumulate buffer
    void Update(bool refresh_accumulate);

    IMFSourceReader *video_reader_ = NULL;
    int width_;
    int height_;
    GLuint texture_id_;
    unsigned char *buffer_;  // RGBA texture data converted from video
    unsigned char *debug_buffer_;  // This is what will be shown in the debug window

    // Average camera color if everything is black/white (should use median?)
    float reference_black_brightness_;
    float reference_white_brightness_;
};

