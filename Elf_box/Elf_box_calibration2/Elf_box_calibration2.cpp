// OpenGL_1_sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Configuration.h"

#include <stdio.h>
#include <tchar.h>
#include <evr.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <fstream>
#include "MFUtility.h"

// Gathered calibration data
#include "calibration.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

//using namespace System;
//using namespace System::IO;
//using namespace System::Runtime::InteropServices;

// For rendering with openGL
extern HINSTANCE instance_handle_;  // main instance
struct WindowHandles {
    WindowHandles() {
        window_handle_ = 0;
        device_context_handle_ = 0;
        resource_context_handle_ = 0;
        window_class_name_ = 0;
    }

    void End() {
        if (resource_context_handle_) {
            wglMakeCurrent(0, 0);
            wglDeleteContext(resource_context_handle_);
        }

        if (window_handle_) {
            if (device_context_handle_) ReleaseDC(window_handle_, device_context_handle_);
            DestroyWindow(window_handle_);
        }

        if (window_class_name_) {
            UnregisterClass(window_class_name_, instance_handle_);
        }
    }

    LPCSTR window_class_name_;
    HWND window_handle_;
    HDC device_context_handle_;
    HGLRC resource_context_handle_;
};

// Global Variables:
HINSTANCE instance_handle_ = 0;  // main instance
WindowHandles main_window_;  // For rendering to beamer
//HWND window_handle_ = 0;  // main window
//HDC device_context_handle_ = 0;  // OpenGL device context
//HGLRC resource_context_handle_= 0;  // OpenGL resource context
static const LPCSTR kMainWindowClassName = "MAIN_CALIBRATOR";
static const LPCSTR kCameraWindowClassName = "CAMERA_CALIBRATOR";

static const PIXELFORMATDESCRIPTOR kPixelFormatDescriptor = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,  // accum
    0,             // zbuffer (32?)
    0,              // stencil!
    0,              // aux
    PFD_MAIN_PLANE,
    0, 0, 0, 0
};

// For Camera detection stuff
WindowHandles camera_window_;
struct Camera {
    Camera() {
        video_reader_ = NULL;
        buffer_ = NULL;
        accumulate_buffer_ = NULL;
    }

    ~Camera() {
        delete[] buffer_;
        buffer_ = NULL;
        delete accumulate_buffer_;
        accumulate_buffer_ = NULL;
        // TODO: Release video_reader_
    }

    // update background image and calculate delta
    void Update(bool update_background, bool refresh_accumulate) {
        if (NULL == accumulate_buffer_) {
            accumulate_buffer_ = new int[width_ * height_ * 4];
        }

        for (int i = 0; i < width_ * height_ * 4; i++) {
            if (refresh_accumulate) {
                accumulate_buffer_[i] = 0;
            }
            double input = static_cast<double>(buffer_[i]);
            double output = input - background_average_[i];
            if (output < 0) output = 0;
            if (output > 255) output = 255;
            const double kAlpha = 0.9f;
            if (update_background) {
                background_average_[i] = kAlpha * background_average_[i] + (1.0f - kAlpha) * input;
            }
            buffer_[i] = static_cast<unsigned char>(output);
            accumulate_buffer_[i] += static_cast<int>(buffer_[i]);

            // Use accumulate buffer instead of buffer
            int out = accumulate_buffer_[i] / 4;
            if (out > 255) out = 255;
            buffer_[i] = static_cast<unsigned char>(out);
        }
    }

    // Sets to 1 if flash is detected in delta image
    bool DetectFlash() {
        int count = 0;
        const float kFlashThreshold = 64;
        const int kCountThreshold = 5000;
        for (int i = 0; i < width_ * height_ * 4; i++) {
            if (buffer_[i] >= kFlashThreshold) count++;
        }
        return (count >= kCountThreshold);
    }

    // Find the brightest spot (single pixel?) in delta image and return its brightness
    float FindBrightestPixel(float *x_pos, float *y_pos) {
        FindBrightestPixel(x_pos, y_pos, accumulate_buffer_, NULL);
    }

    float FindBrightestPixel(float *x_pos, float *y_pos, int *buffer, int *multiply_buffer) {
        *x_pos = 0.0f;
        *y_pos = 0.0f;
        int brightest = -1;
        int brightest_x = 0;
        int brightest_y = 0;
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                int brightness = 0;
                for (int colors = 0; colors < 3; colors++) {
                    int index = (y * width_ + x) * 4 + colors;
                    int factor = 1;
                    if (multiply_buffer) {
                        factor = multiply_buffer[index];
                    }
                    brightness += buffer[index] * factor;
                }
                if (brightness > brightest) {
                    brightest_x = x;
                    brightest_y = y;
                    brightest = brightness;
                }
            }
        }

        *x_pos = static_cast<float>(brightest_x) / width_ * 2.0f - 1.0f;
        *y_pos = static_cast<float>(brightest_y) / height_ * 2.0f - 1.0f;
        return static_cast<float>(brightest) / (255 * 3);
    }

    IMFSourceReader *video_reader_ = NULL;
    int width_;
    int height_;
    GLuint texture_id_;
    unsigned char *buffer_;  // RGBA texture data converted from video
    int *accumulate_buffer_;  // RGBA sum of values over time, if no background update is done

    // low-pass filtered color representing background image
    double *background_average_;
};
Camera camera_;

// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Some camera testing
int InitCamera() {
    std::string intent = "WEBCAM_DEVICE_INDEX = " + std::to_string(WEBCAM_DEVICE_INDEX) + "\n";
    intent += "MEDIA_TYPE_INDEX = " + std::to_string(MEDIA_TYPE_INDEX) + "\n";
#ifdef LIST_DEVICES
    MessageBox(NULL, intent.c_str(), "Intent for Camera usage", MB_OK);
#endif

    IMFActivate **videoDevices = NULL;
    UINT32 videoDeviceCount = 0;
    IMFMediaSource *videoSource = NULL;
    IMFAttributes *videoConfig = NULL;
    IMFMediaType*videoSourceOutputType = NULL;
    IMFMediaType *pSrcOutMediaType = NULL;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    MFStartup(MF_VERSION);

    // Get the first available webcam.
    CHECK_HR(MFCreateAttributes(&videoConfig, 1), "Error creating video configuation.\n");

    // Request video capture devices.
    CHECK_HR(videoConfig->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID), "Error initialising video configuration object.");

    // Enumerate devices
    CHECK_HR(MFEnumDeviceSources(videoConfig, &videoDevices, &videoDeviceCount), "Error enumerating devices");

#ifdef LIST_DEVICES
    ListCameras(videoDevices, videoDeviceCount);
#endif

    if (WEBCAM_DEVICE_INDEX >= videoDeviceCount) {
        MessageBox(NULL, "Not enough cameras installed.", "Camera not fount", MB_OK);
        return -1;
    }
    CHECK_HR(videoDevices[WEBCAM_DEVICE_INDEX]->ActivateObject(IID_PPV_ARGS(&videoSource)), "Error activating video device.\n");

    // Create a source reader.
    CHECK_HR(MFCreateSourceReaderFromMediaSource(
        videoSource,
        videoConfig,
        &(camera_.video_reader_)), "Error creating video source reader.\n");

    // Show a list of all the subtypes
#ifdef LIST_DEVICES
    ListModes(camera_.video_reader_);
#endif

#if 1
    // Choose one
    CHECK_HR(camera_.video_reader_->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, MEDIA_TYPE_INDEX, &pSrcOutMediaType),
             "Error retreaving selected media type");
#else
    // Note the webcam needs to support this media type. The list of media types supported can be obtained using the ListTypes function in MFUtility.h.
    MFCreateMediaType(&pSrcOutMediaType);
    pSrcOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pSrcOutMediaType->SetGUID(MF_MT_SUBTYPE, WMMEDIASUBTYPE_I420);
    MFSetAttributeSize(pSrcOutMediaType, MF_MT_FRAME_SIZE, 640, 480);
#endif

    CHECK_HR(camera_.video_reader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pSrcOutMediaType), "Failed to set media type on source reader.\n");

    // Get relevant information
    UINT64 value;
    CHECK_HR(pSrcOutMediaType->GetUINT64(MF_MT_FRAME_SIZE, &value), "Could not get video dimension");
    camera_.width_ = HI32(value);
    camera_.height_ = LO32(value);
    int texture_size = camera_.width_ * camera_.height_ * 4;
    camera_.buffer_ = new unsigned char[texture_size];
    for (int i = 0; i < texture_size; i++) {
        camera_.buffer_[i] = 0;
    }
    camera_.background_average_ = new double[texture_size];
    for (int i = 0; i < texture_size; i++) {
        camera_.background_average_[i] = static_cast<double>(camera_.buffer_[i]);
    }

    // Create OpenGL texture suitable for the camera picture
    glGenTextures(1, &(camera_.texture_id_));
    glBindTexture(GL_TEXTURE_2D, camera_.texture_id_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        camera_.width_, camera_.height_,
        0, GL_BGRA, GL_UNSIGNED_BYTE, camera_.buffer_);

    return 0;
}

// Clip in range 0..255
int clip(int x) {
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
}

int GetFrame(bool update_background, bool refresh_accumulate) {
    IMFSample *videoSample = NULL;
    DWORD streamIndex, flags;
    LONGLONG llVideoTimeStamp;

    CHECK_HR(camera_.video_reader_->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,                              // Flags.
        &streamIndex,                   // Receives the actual stream index. 
        &flags,                         // Receives status flags.
        &llVideoTimeStamp,              // Receives the time stamp.
        &videoSample                    // Receives the sample or NULL.
        ), "Error reading video sample.");

    if (flags & MF_SOURCE_READERF_STREAMTICK)
    {
        //printf("Stream tick.\n");
    }

    if (videoSample)
    {
        //CHECK_HR(videoSample->SetSampleTime(llVideoTimeStamp), "Error setting the video sample time.\n");
        //CHECK_HR(videoSample->GetSampleDuration(&llSampleDuration), "Error getting video sample duration.\n");

        IMFMediaBuffer *buf = NULL;
        DWORD bufLength;
        CHECK_HR(videoSample->ConvertToContiguousBuffer(&buf), "ConvertToContiguousBuffer failed.\n");
        CHECK_HR(buf->GetCurrentLength(&bufLength), "Get buffer length failed.\n");

        //printf("Sample length %i.\n", bufLength);

        byte *byteBuffer;
        DWORD buffCurrLen = 0;
        DWORD buffMaxLen = 0;
        buf->Lock(&byteBuffer, &buffMaxLen, &buffCurrLen);

        // Convert to RGBA in camera_.buffer
        unsigned char *ptrOut = camera_.buffer_;
        byte *ptrIn = byteBuffer;
        for (int y = 0; y < camera_.height_; y++) {
            for (int x = 0; x < camera_.width_; x += 2) {
                int y0 = ptrIn[0];
                int u0 = ptrIn[1];
                int y1 = ptrIn[2];
                int v0 = ptrIn[3];
                ptrIn += 4;
                int c = y0 - 16;
                int d = u0 - 128;
                int e = v0 - 128;
                ptrOut[0] = clip(( 298 * c + 516 * d + 128) >> 8); // blue
                ptrOut[1] = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
                ptrOut[2] = clip(( 298 * c + 409 * e + 128) >> 8); // red
                ptrOut[3] = 255;
                c = y1 - 16;
                ptrOut[4] = clip(( 298 * c + 516 * d + 128) >> 8); // blue
                ptrOut[5] = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
                ptrOut[6] = clip(( 298 * c + 409 * e + 128) >> 8); // red
                ptrOut[7] = 255;

                //ptrOut[0] = ptrOut[1] = ptrOut[2] = x;
                //ptrOut[4] = ptrOut[5] = ptrOut[6] = x;

                ptrOut += 8;
            }
        }

        // Update with background
        camera_.Update(update_background, refresh_accumulate);

        // Update texture
        glEnable(GL_TEXTURE_2D);				// Enable Texture Mapping
        glBindTexture(GL_TEXTURE_2D, camera_.texture_id_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camera_.width_, camera_.height_,
            GL_BGRA, GL_UNSIGNED_BYTE, camera_.buffer_);

        buf->Release();
        videoSample->Release();
    }

    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE instance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Perform application initialization:
    if (!InitInstance(instance, nCmdShow, false, 0, kMainWindowClassName, &main_window_)) {
        return FALSE;
    }

    // NEHE MULTISAMPLE
    bool arbMultisampleSupported = false;
    int arbMultisampleFormat = 0;
    // Get Our Pixel Format
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    HDC hDC = main_window_.device_context_handle_;
    int pixelFormat;
    int valid;
    UINT numFormats;
    float fAttributes[] = { 0, 0 };
    //const int kMaxNumMultisamples = 16;
    const int kMaxNumMultisamples = 8;
    // These Attributes Are The Bits We Want To Test For In Our Sample
    // Everything Is Pretty Standard, The Only One We Want To
    // Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
    // These Two Are Going To Do The Main Testing For Whether Or Not
    // We Support Multisampling On This Hardware
    int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
        WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 0, //32
        WGL_STENCIL_BITS_ARB, 0,
        WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
        WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
        WGL_SAMPLES_ARB, kMaxNumMultisamples,
        0,0 };
    // First check with maximum number of multisamples
    valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
    // As long as it didn't work, try half the number of multisamples
    while ((!valid || numFormats < 1) && iAttributes[19] > 2) {
        iAttributes[19] /= 2;
        valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
    }
    if (valid && numFormats >= 1) {
        arbMultisampleSupported = true;
        arbMultisampleFormat = pixelFormat;
    }

    if (arbMultisampleSupported) {
        main_window_.End();
        // Remove all messages...
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat, kMainWindowClassName, &main_window_)) {
            return FALSE;
        }
    }
    glEnable(GL_MULTISAMPLE_ARB);

    if (arbMultisampleSupported) {
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat, kCameraWindowClassName, &camera_window_)) {
            return FALSE;
        }
    } else {
        if (!InitInstance(instance, nCmdShow, false, 0, kCameraWindowClassName, &camera_window_)) {
            return FALSE;
        }
    }

    // The main container holding all the thread information
    const char *kAutoSaveFileName = "auto_save.cali";
    FILE *fid;
    if (!fopen_s(&fid, kAutoSaveFileName, "rb")) {
        fclose(fid);
    }

    // Main message loop:
    long start_time = timeGetTime();
    long last_flash = 0;
    bool done = false;
    int num_accumulated = 0;
    int index = 0;  // Index of currently handled point/line

#ifdef USE_CAMERA
    if (InitCamera() < 0) return -1;
#endif

    MSG msg;
    while (!done) {
        long cur_time = timeGetTime() - start_time;
        float time_seconds = 0.001f * static_cast<float>(cur_time);

        // Check whether background shall be 
        //const int kCalibrationDelay = 0;
        const int kCalibrationDelay = CALIBRATION_DELAY;
        bool update_background = true;  // Flag whether camera brightness is re-calibrated
        bool show_image = false;  // Flag whether the calibration image is shown
        bool check_calibration = false;  // Use accumulate buffer to check
        bool refresh_accumulate = true;  // Flag that deletes accumulate buffer in camera

        if (cur_time - last_flash > USE_BACKGROUND_UPDATE &&
            (index > 0 || cur_time > kCalibrationDelay)) {
            update_background = false;
            show_image = true;
        }

        if (cur_time - last_flash > USE_LATENCY + USE_BACKGROUND_UPDATE &&
            (index > 0 || cur_time > kCalibrationDelay)) {
            refresh_accumulate = false;
            num_accumulated++;
            if (num_accumulated > NUM_ACCUMULATE) {
                num_accumulated = 0;
                check_calibration = true;
                show_image = false;
            }
        }

#ifdef USE_CAMERA
        GetFrame(update_background, refresh_accumulate);  // Check return value?
#endif

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Draw main window:
        if (!wglMakeCurrent(main_window_.device_context_handle_, main_window_.resource_context_handle_)) return false;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        
        // Flash for latency testing
#ifdef LATENCY_MEASUREMENT
        const int kWaitFlashTime = 1000;
        if (cur_time - last_flash >= kWaitFlashTime) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (camera_.DetectFlash()) {
                if (!fopen_s(&fid, "latency.ms.txt", "ab")) {
                    fprintf(fid, "%d\n", cur_time - last_flash - kWaitFlashTime);
                    fclose(fid);
                }
                last_flash = cur_time;
            }
        }
#endif

        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LESS);

        glLoadIdentity();  // Reset The View

#if defined(MATRIX_CALIBRATION) || defined(POINT_CALIBRATION)
        static float brightness[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
        static float calibration_x[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
        static float calibration_y[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
#endif

#ifdef MATRIX_CALIBRATION
        static int *row_accumulate_buffer[CALIBRATION_X_RESOLUTION];
        static bool do_row_calibration = false;

        if (do_row_calibration) {  // Check for second pass (row calibration)
            if (check_calibration) {
                // This is the last time that the row is shown - check it
                if (index >= 0 && index < CALIBRATION_Z_RESOLUTION) {
                    for (int column = 0; column < CALIBRATION_X_RESOLUTION; column++) {
                        int x_index = column;
                        int z_index = index;
                        brightness[z_index][x_index] = camera_.FindBrightestPixel(
                            &(calibration_x[z_index][x_index]), &(calibration_y[z_index][x_index]),
                            camera_.accumulate_buffer_, row_accumulate_buffer[column]);
                    }
                }
                last_flash = cur_time;
                index++;
                if (index >= CALIBRATION_Z_RESOLUTION) {
                    done = true;
                }
            }

            if (show_image) {
                if (index >= 0 && index < CALIBRATION_Z_RESOLUTION) {
                    float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                    float z_pos = index * depth - 1.0f;
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glRectf(-1.0f, z_pos, 1.0f, z_pos + depth);
                }
            }

        } else {  // Do first pass (columns)
            
            if (check_calibration) {
                // This is the last time that the column is shown - check it
                if (index >= 0 && index < CALIBRATION_X_RESOLUTION) {
                    // Save image...
                    row_accumulate_buffer[index] = new int[camera_.width_ * camera_.height_ * 4];
                    for (int i = 0; i < camera_.width_ * camera_.height_ * 4; i++) {
                        row_accumulate_buffer[index][i] = camera_.accumulate_buffer_[i];
                    }
                }
                last_flash = cur_time;
                index++;
                // All columns processed, go to row mode
                if (index >= CALIBRATION_X_RESOLUTION) {
                    do_row_calibration = true;
                    index = 0;
                }
            }

            if (show_image) {
                if (index >= 0 && index < CALIBRATION_X_RESOLUTION) {
                    float width = 2.0f / CALIBRATION_X_RESOLUTION;
                    float x_pos = index * width - 1.0f;
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glRectf(x_pos, -1.0f, x_pos + width, 1.0f);
                }
            }
        }
#endif

#ifdef POINT_CALIBRATION
        static float brightness[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
        static float calibration_x[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
        static float calibration_y[CALIBRATION_Z_RESOLUTION][CALIBRATION_X_RESOLUTION];
        static int last_index = -1;  // I don't think this works with the new timing...
        
        if (check_calibration) {
            // This is the last time that the last spot is shown, check it
            if (last_index >= 0 && last_index < CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION) {
                int x_index = (last_index % CALIBRATION_X_RESOLUTION);
                int z_index = (last_index / CALIBRATION_Z_RESOLUTION);
                brightness[z_index][x_index] = camera_.FindBrightestPixel(
                    &(calibration_x[z_index][x_index]), &(calibration_y[z_index][x_index]));
            }
            last_index = index;
            last_flash = cur_time;
            index++;
            if (last_index >= CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION) {
                done = true;
            }
        }
        
        if (show_image) {
            if (index >= 0 && index < CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION) {
                float width = 2.0f / CALIBRATION_X_RESOLUTION;
                float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                float x_pos = (index % CALIBRATION_X_RESOLUTION) * width - 1.0f;
                float z_pos = (index / CALIBRATION_Z_RESOLUTION) * depth - 1.0f;
                glColor3f(0.2f, 0.2f, 0.2f);
                glRectf(x_pos, z_pos, x_pos + width, z_pos + depth);
            }
        }
#endif

#if defined(MATRIX_CALIBRATION) || defined(POINT_CALIBRATION)
        // Save data
        if (done) {
            if (!fopen_s(&fid, "calibration.h", "wb")) {
                fprintf(fid, "float calibration_brightness_[%d][%d] = {", CALIBRATION_Z_RESOLUTION, CALIBRATION_X_RESOLUTION);
                for (int z = 0; z < CALIBRATION_Z_RESOLUTION; z++) {
                    fprintf(fid, "\n  {");
                    for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                        fprintf(fid, "%ff, ", brightness[z][x]);
                    }
                    fprintf(fid, "},");
                }
                fprintf(fid, "\n};\n\n");
                fprintf(fid, "float calibration_x_[%d][%d] = {", CALIBRATION_Z_RESOLUTION, CALIBRATION_X_RESOLUTION);
                for (int z = 0; z < CALIBRATION_Z_RESOLUTION; z++) {
                    fprintf(fid, "\n  {");
                    for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                        fprintf(fid, "%ff, ", calibration_x[z][x]);
                    }
                    fprintf(fid, "},");
                }
                fprintf(fid, "\n};\n\n");
                fprintf(fid, "float calibration_y_[%d][%d] = {", CALIBRATION_Z_RESOLUTION, CALIBRATION_X_RESOLUTION);
                for (int z = 0; z < CALIBRATION_Z_RESOLUTION; z++) {
                    fprintf(fid, "\n  {");
                    for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                        fprintf(fid, "%ff, ", calibration_y[z][x]);
                    }
                    fprintf(fid, "},");
                }
                fprintf(fid, "\n};\n\n");
                fclose(fid);
            }
        }
#endif

#ifdef SHOW_HEIGHT
        float maximum_brightness = 0.0;
        for (int i = 0; i < CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION; i++) {
            if (calibration_brightness_[0][i] > maximum_brightness) {
                maximum_brightness = calibration_brightness_[0][i];
            }
        }

        for (int z = 0; z < CALIBRATION_Z_RESOLUTION; z++) {
            for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                float width = 2.0f / CALIBRATION_X_RESOLUTION;
                float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                //float col_range = calibration_y_[z][x] * 0.5f + 0.5f;
                float show_height = sinf(static_cast<float>(cur_time) * 0.0003f);
                float distance = fabsf(show_height - calibration_y_[z][x]);
                float brightness = calibration_brightness_[z][x] / maximum_brightness * 16.0f;
                if (brightness > 1.0f) brightness = 1.0f;
                brightness *= 1.0f - (distance * 10.0f);
                if (brightness < 0.0f) brightness = 0.0f;
                //float red = col_range * brightness;
                //float green = (1.0f - col_range) * brightness;
                //float blue = 0.0f * brightness;
                //glColor3f(red, green, blue);
                glColor3f(brightness, brightness, brightness);
                float x_pos = x * width - 1.0f;
                float z_pos = z * depth - 1.0f;
                glRectf(x_pos, z_pos, x_pos + width, z_pos + depth);
            }
        }
#endif

        SwapBuffers(main_window_.device_context_handle_);

        // Render camera window
        if (!wglMakeCurrent(camera_window_.device_context_handle_, camera_window_.resource_context_handle_)) return false;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set transformation matrix to do aspect ratio adjustment
        glLoadIdentity();  // Reset The View

        glEnable(GL_TEXTURE_2D);				// Enable Texture Mapping
        glBindTexture(GL_TEXTURE_2D, camera_.texture_id_);

        // Rendering of the camera rect
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(+1.0f, 1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(+1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(+1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, +1.0f);
        glEnd();

        // Show the last calibration point
#ifdef POINT_CALIBRATION
        if (last_index - 1 >= 0 && last_index - 1< CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION) {
            glColor3f(1.0f, 0.0f, 0.0f);
            int x_index = ((last_index - 1) % CALIBRATION_X_RESOLUTION);
            int z_index = ((last_index - 1) / CALIBRATION_Z_RESOLUTION);
            float x = calibration_x[z_index][x_index];
            float y = -calibration_y[z_index][x_index];
            glDisable(GL_TEXTURE_2D);
            glRectf(x - 0.002f, y - 0.002f, x + 0.002f, y + 0.002f);
        }
#endif

        SwapBuffers(camera_window_.device_context_handle_);
    }

    // Cleanup
    main_window_.End();
    camera_window_.End();

    return (int) msg.wParam;
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles) {
    instance_handle_ = instance; // Store instance handle in our global variable
    DWORD window_style;
    RECT window_rectangle;
    unsigned int	pixel_format;

    WNDCLASSA window_class;

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WndProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = window_class_name;

    RegisterClassA(&window_class);
    handles->window_class_name_ = window_class_name;

    // Set default window size
    window_rectangle.left   = 0;
    window_rectangle.top    = 0;
    window_rectangle.right  = XRES;
    window_rectangle.bottom = YRES;

    window_style   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    AdjustWindowRect(&window_rectangle, window_style, 0);

    handles->window_handle_ = CreateWindowA(window_class_name, "OpenGL 1 sample", window_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_rectangle.right - window_rectangle.left,
        window_rectangle.bottom - window_rectangle.top,
        0,  // Parent window
        0,  // Menu
        instance_handle_,
        0);  // lParam
    if (!handles->window_handle_) return false;

    if (!(handles->device_context_handle_ = GetDC(handles->window_handle_))) return false;

    if (!use_custom_pixel_format) {
        if (!(pixel_format = ChoosePixelFormat(handles->device_context_handle_, &kPixelFormatDescriptor))) {
            return false;
        }
        if (!SetPixelFormat(handles->device_context_handle_, pixel_format, &kPixelFormatDescriptor)) {
            return false;
        }
    } else {
        if (!SetPixelFormat(handles->device_context_handle_, custom_pixel_format, &kPixelFormatDescriptor)) {
            return false;
        }
    }

    // create basic context
    if (!(handles->resource_context_handle_ = wglCreateContext(handles->device_context_handle_))) return false;
    if (!wglMakeCurrent(handles->device_context_handle_, handles->resource_context_handle_)) return false;

    ShowWindow(handles->window_handle_, nCmdShow);
    UpdateWindow(handles->window_handle_);

    return true;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Don't turn power off
    if (message == WM_SYSCOMMAND && (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER)) {
        return 0;
    }

    // Shut down command
    if (message == WM_CLOSE || message == WM_DESTROY ||
        (message == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        PostQuitMessage(0);
        return 0;
    }

    // Handle editing messages
    if (message == WM_KEYDOWN) {
        switch (wParam) {
        case 'M':
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                RECT window_rect;
                // TODO: Minimization again.
                SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                ShowWindow(hWnd, SW_MAXIMIZE);
                GetClientRect(hWnd, &window_rect);
                if (hWnd == main_window_.window_handle_) {
                    wglMakeCurrent(main_window_.device_context_handle_, main_window_.resource_context_handle_);
                } else {
                    wglMakeCurrent(camera_window_.device_context_handle_, camera_window_.resource_context_handle_);
                }
                glViewport(0, 0,
                    window_rect.right - window_rect.left,
                    abs(window_rect.bottom - window_rect.top));
                ShowCursor(false);
            }
            break;
        default:            
            break;
        }
    }

    if (message == WM_CHAR) {
        if (wParam == VK_ESCAPE ) {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Called by Parameter.cpp if a key has changed its value
void registerParameterChange(int keyID) {
}