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
IMFSourceReader *video_reader_ = NULL;
WindowHandles camera_window_;

// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Some camera testing
int InitCamera() {
    const int WEBCAM_DEVICE_INDEX = 1;	// <--- Set to 0 to use default system webcam.
    const int MEDIA_TYPE_INDEX = 43;  // 160x120, YUY2, 30 FPS, ~9MBPS

    std::string intent = "WEBCAM_DEVICE_INDEX = " + std::to_string(WEBCAM_DEVICE_INDEX) + "\n";
    intent += "MEDIA_TYPE_INDEX = " + std::to_string(MEDIA_TYPE_INDEX) + "\n";
    MessageBox(NULL, intent.c_str(), "Intent for Camera usage", MB_OK);

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

    ListCameras(videoDevices, videoDeviceCount);

    CHECK_HR(videoDevices[WEBCAM_DEVICE_INDEX]->ActivateObject(IID_PPV_ARGS(&videoSource)), "Error activating video device.\n");

    // Create a source reader.
    CHECK_HR(MFCreateSourceReaderFromMediaSource(
        videoSource,
        videoConfig,
        &video_reader_), "Error creating video source reader.\n");

    // Show a list of all the subtypes
    ListModes(video_reader_);

#if 1
    // Choose one
    CHECK_HR(video_reader_->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, MEDIA_TYPE_INDEX, &pSrcOutMediaType),
             "Error retreaving selected media type");
#else
    // Note the webcam needs to support this media type. The list of media types supported can be obtained using the ListTypes function in MFUtility.h.
    MFCreateMediaType(&pSrcOutMediaType);
    pSrcOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pSrcOutMediaType->SetGUID(MF_MT_SUBTYPE, WMMEDIASUBTYPE_I420);
    MFSetAttributeSize(pSrcOutMediaType, MF_MT_FRAME_SIZE, 640, 480);
#endif

    CHECK_HR(video_reader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pSrcOutMediaType), "Failed to set media type on source reader.\n");

    return 0;
}

int GetFrame() {
    IMFSample *videoSample = NULL;
    DWORD streamIndex, flags;
    LONGLONG llVideoTimeStamp, llSampleDuration;
    int sampleCount = 0;
    const int SAMPLE_COUNT = 10;
    std::ofstream outputBuffer("rawframes.yuv", std::ios::out | std::ios::binary);

    while (sampleCount <= SAMPLE_COUNT)
    {
        CHECK_HR(video_reader_->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,                              // Flags.
            &streamIndex,                   // Receives the actual stream index. 
            &flags,                         // Receives status flags.
            &llVideoTimeStamp,              // Receives the time stamp.
            &videoSample                    // Receives the sample or NULL.
            ), "Error reading video sample.");

        if (flags & MF_SOURCE_READERF_STREAMTICK)
        {
            printf("Stream tick.\n");
        }

        if (videoSample)
        {
            printf("Writing sample %i.\n", sampleCount);

            CHECK_HR(videoSample->SetSampleTime(llVideoTimeStamp), "Error setting the video sample time.\n");
            CHECK_HR(videoSample->GetSampleDuration(&llSampleDuration), "Error getting video sample duration.\n");

            IMFMediaBuffer *buf = NULL;
            DWORD bufLength;
            CHECK_HR(videoSample->ConvertToContiguousBuffer(&buf), "ConvertToContiguousBuffer failed.\n");
            CHECK_HR(buf->GetCurrentLength(&bufLength), "Get buffer length failed.\n");

            printf("Sample length %i.\n", bufLength);

            byte *byteBuffer;
            DWORD buffCurrLen = 0;
            DWORD buffMaxLen = 0;
            buf->Lock(&byteBuffer, &buffMaxLen, &buffCurrLen);

            outputBuffer.write((char *)byteBuffer, bufLength);

            buf->Release();
        }

        sampleCount++;
    }

    outputBuffer.close();

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
    bool done = false;

    if (InitCamera() < 0) return -1;
    if (GetFrame() < 0) return -1;

    MSG msg;
    while (!done) {
        // Create one thread as the currently edited one:
        long cur_time = timeGetTime() - start_time;
        float time_seconds = 0.001f * static_cast<float>(cur_time);

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Draw main window:
        if (!wglMakeCurrent(main_window_.device_context_handle_, main_window_.resource_context_handle_)) return false;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LESS);

        // Set transformation matrix to do aspect ratio adjustment
        RECT window_rectangle;
        GetWindowRect(main_window_.window_handle_, &window_rectangle);
        float stretch_matrix[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        int width = window_rectangle.right - window_rectangle.left;
        int height = window_rectangle.bottom - window_rectangle.top;
        stretch_matrix[1][1] = static_cast<float>(width) / static_cast<float>(height);
        glLoadIdentity();  // Reset The View
        glLoadMatrixf(stretch_matrix[0]);

        // TEST rendering
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.6f, 0.3f);
        glVertex2f(-0.2f, 0.6f);
        glVertex2f(0.7f, 0.3f);
        glVertex2f(0.2f, -0.6f);
        glEnd();

        SwapBuffers(main_window_.device_context_handle_);

        // Render camera window
        if (!wglMakeCurrent(camera_window_.device_context_handle_, camera_window_.resource_context_handle_)) return false;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set transformation matrix to do aspect ratio adjustment
        glLoadIdentity();  // Reset The View

        // TEST rendering
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 0.6f, 1.0f);
        glVertex2f(-0.7f, 0.3f);
        glVertex2f(0.9f, 0.1f);
        glVertex2f(0.2f, -0.2f);
        glEnd();

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
        (GetSystemMetrics(SM_CXSCREEN) - window_rectangle.right + window_rectangle.left) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - window_rectangle.bottom + window_rectangle.top) / 2,
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