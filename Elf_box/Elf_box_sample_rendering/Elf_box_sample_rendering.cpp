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

// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Clip in range 0..255
int clip(int x) {
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
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
#ifdef USE_MULTISAMPLE
    if (valid && numFormats >= 1) {
        arbMultisampleSupported = true;
        arbMultisampleFormat = pixelFormat;
    }
#endif

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

    MSG msg;
    while (!done) {
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

        glLoadIdentity();  // Reset The View

#ifdef SHOW_HEIGHT
        float maximum_brightness = 0.0;
        for (int i = 0; i < CALIBRATION_X_RESOLUTION * CALIBRATION_Z_RESOLUTION; i++) {
            if (calibration_brightness_[0][i] > maximum_brightness) {
                maximum_brightness = calibration_brightness_[0][i];
            }
        }

        for (int z = 0; (float)z < CALIBRATION_Z_RESOLUTION * MAX_Z_DRAW; z++) {
            for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                if (calibration_brightness_[z][x] >= MIN_DRAW_BRIGHTNESS)
                {
                    float width = 2.0f / CALIBRATION_X_RESOLUTION;
                    float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                    //float col_range = calibration_y_[z][x] * 0.5f + 0.5f;
                    float show_height = sinf(static_cast<float>(cur_time) * 0.0003f);
                    float distance = fabsf(show_height - calibration_y_[z][x]);
                    //float brightness = calibration_brightness_[z][x] / maximum_brightness * 16.0f;
                    float brightness = 1.0f;
                    if (calibration_brightness_[z][x] < 0.5f) brightness = 0.0f;
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
        }
#endif

        // Scaling to make it somewhat cubic
        const float kYScale = 2.5f;
        const float kXScale = 2.5f;
        const float kZScale = 2.0f;
        float ftime = static_cast<float>(cur_time) * 0.001f;

#ifdef SHOW_BALL
        float ball_x = sinf(ftime * 0.37f + 1.3f) +
            0.75f * sinf(ftime * 0.52f + 2.1f) +
            0.5f * sinf(ftime * 0.73f + 0.2f);
        float ball_y = sinf(ftime * 0.41f + 1.1f) +
            0.75f * sinf(ftime * 0.61f + 0.9f) +
            0.5f * sinf(ftime * 0.71f + 3.1f);
        float ball_z = sinf(ftime * 0.32f + 0.1f) +
            0.75f * sinf(ftime * 0.48f + 2.9f) +
            0.5f * sinf(ftime * 0.91f + 1.7f);

        for (int z = 0; (float)z < CALIBRATION_Z_RESOLUTION * MAX_Z_DRAW; z++) {
            for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                if (calibration_brightness_[z][x] >= MIN_DRAW_BRIGHTNESS)
                {
                    float width = 2.0f / CALIBRATION_X_RESOLUTION;
                    float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                    float x_pos = x * width - 1.0f;
                    float z_pos = z * depth - 1.0f;
                    float y_pos = calibration_y_[z][x];
                    float scaled_x = x_pos * kXScale;
                    float scaled_y = y_pos * kYScale;
                    float scaled_z = z_pos * kZScale;

                    float distance = sqrtf((ball_x - scaled_x) * (ball_x - scaled_x) +
                        (ball_y - scaled_y) * (ball_y - scaled_y) +
                        (ball_z - scaled_z) * (ball_z - scaled_z));
                    //float brightness = calibration_brightness_[z][x];
                    //brightness *= brightness;
                    float brightness = 1.0f;
                    if (calibration_brightness_[z][x] < 0.5f) brightness = 0.0f;
                    if (brightness > 1.0f) brightness = 1.0f;
                    if (distance > 1.0f) brightness = 0.0f;
                    glColor3f(brightness, brightness, brightness);
                    glRectf(x_pos, z_pos, x_pos + width, z_pos + depth);
                }
            }
        }
#endif

#ifdef SHOW_RAINBOW_BALL
        float ball_x = sinf(ftime * 0.37f + 1.3f) +
            0.75f * sinf(ftime * 0.52f + 2.1f) +
            0.5f * sinf(ftime * 0.73f + 0.2f);
        float ball_y = sinf(ftime * 0.41f + 1.1f) +
            0.75f * sinf(ftime * 0.61f + 0.9f) +
            0.5f * sinf(ftime * 0.71f + 3.1f);
        float ball_z = sinf(ftime * 0.32f + 0.1f) +
            0.75f * sinf(ftime * 0.48f + 2.9f) +
            0.5f * sinf(ftime * 0.91f + 1.7f);

        for (int z = 0; (float)z < CALIBRATION_Z_RESOLUTION * MAX_Z_DRAW; z++) {
            for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++) {
                if (calibration_brightness_[z][x] >= MIN_DRAW_BRIGHTNESS)
                {
                    float width = 2.0f / CALIBRATION_X_RESOLUTION;
                    float depth = 2.0f / CALIBRATION_Z_RESOLUTION;
                    float x_pos = x * width - 1.0f;
                    float z_pos = z * depth - 1.0f;
                    float y_pos = calibration_y_[z][x];
                    float scaled_x = x_pos * kXScale;
                    float scaled_y = y_pos * kYScale;
                    float scaled_z = z_pos * kZScale;

                    float distance = sqrtf((ball_x - scaled_x) * (ball_x - scaled_x) +
                        (ball_y - scaled_y) * (ball_y - scaled_y) +
                        (ball_z - scaled_z) * (ball_z - scaled_z));

                    const float kDistancePhase = 6.0f;
                    const float kTimePhase = 7.0f;
                    float phase = distance * kDistancePhase - ftime * kTimePhase;
                    int index = static_cast<int>(phase / 3.14153f / 2.0f);

                    float red = 0.7f + 0.5f * sinf(index * 3.0f);
                    float green = 0.7f + 0.5f * sinf(index * 2.3f);
                    float blue = 0.7f + 0.5f * sinf(index * 4.6f);
                    
                    float alpha = 0.5f + sinf(index * 0.5f);
                    if (alpha < 0.0f) alpha = 0.0f; else alpha = 1.0f;
                    alpha = 2.0f * alpha / (1.0f + distance * distance);

                    //float brightness = calibration_brightness_[z][x] / maximum_brightness * 16.0f;
                    float brightness = alpha;
                    //if (calibration_brightness_[z][x] < 0.5f) brightness = 0.0f;
                    if (brightness > 1.0f) brightness = 1.0f;
                    float rainbow = 2.0f * sin(phase) - 0.75f;
                    if (rainbow < 0.0f) rainbow = 0.0f;
                    //if (rainbow > 1.0f) rainbow = 1.0f;
                    brightness *= rainbow;
                    float dist_brightness = 1.5f / (distance + 0.3f);
                    if (dist_brightness < 0.0f) dist_brightness = 0.0f;
                    //if (dist_brightness > 1.0f) dist_brightness = 1.0f;
                    brightness *= dist_brightness;
                    glColor3f(red * brightness, green * brightness, blue * brightness);
                    glRectf(x_pos, z_pos, x_pos + width, z_pos + depth);
                }
            }
        }
#endif

        SwapBuffers(main_window_.device_context_handle_);
    }

    // Cleanup
    main_window_.End();

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
                wglMakeCurrent(main_window_.device_context_handle_, main_window_.resource_context_handle_);
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