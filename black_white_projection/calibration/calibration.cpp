// OpenGL_1_sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Configuration.h"

// Gathered calibration data
#include "Calibrator.h"

// For rendering with openGL
extern HINSTANCE instance_handle_;  // main instance
struct WindowHandles
{
    WindowHandles()
    {
        window_handle_ = 0;
        device_context_handle_ = 0;
        resource_context_handle_ = 0;
        window_class_name_ = 0;
    }

    void End()
    {
        if (resource_context_handle_)
        {
            wglMakeCurrent(0, 0);
            wglDeleteContext(resource_context_handle_);
        }

        if (window_handle_)
        {
            if (device_context_handle_) ReleaseDC(window_handle_, device_context_handle_);
            DestroyWindow(window_handle_);
        }

        if (window_class_name_)
        {
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
WindowHandles debug_window_;
Calibrator calibrator_;

// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles,
                  const char *window_title);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE instance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Perform application initialization:
    if (!InitInstance(instance, nCmdShow, false, 0, kMainWindowClassName, &main_window_, "On Projector"))
    {
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
    const int kMaxNumMultisamples = 1;
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
#if USE_MULTISAMPLE
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
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat, kMainWindowClassName, &main_window_, "On Projector")) {
            return FALSE;
        }
    }
    glEnable(GL_MULTISAMPLE_ARB);

    if (arbMultisampleSupported) {
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat, kCameraWindowClassName, &debug_window_, "For debugging")) {
            return FALSE;
        }
    } else {
        if (!InitInstance(instance, nCmdShow, false, 0, kCameraWindowClassName, &debug_window_, "For debugging")) {
            return FALSE;
        }
    }

    // Main message loop:
    if (!calibrator_.Init(main_window_.device_context_handle_, main_window_.resource_context_handle_,
                          debug_window_.device_context_handle_, debug_window_.resource_context_handle_)) return -1;

    if (!calibrator_.Calibrate(main_window_.device_context_handle_, main_window_.resource_context_handle_,
                               debug_window_.device_context_handle_, debug_window_.resource_context_handle_)) return -1;

    // Cleanup
    main_window_.End();
    debug_window_.End();

    return 0;
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
                  WindowHandles *handles,
                  const char *window_title) {
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

    handles->window_handle_ = CreateWindowA(window_class_name, window_title, window_style,
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
                    wglMakeCurrent(debug_window_.device_context_handle_, debug_window_.resource_context_handle_);
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