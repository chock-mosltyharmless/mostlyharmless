// OpenGL_1_sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Configuration.h"
#include "ThreadInformation.h"

// Global Variables:
HINSTANCE instance_handle_ = 0;  // main instance
HWND window_handle_ = 0;  // main window
HDC device_context_handle_ = 0;  // OpenGL device context
HGLRC resource_context_handle_= 0;  // OpenGL resource context
static const LPCSTR kWindowClassName = "OPENGL_1_SAMPLE";
static const PIXELFORMATDESCRIPTOR kPixelFormatDescriptor = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,  // accum
    32,             // zbuffer
    0,              // stencil!
    0,              // aux
    PFD_MAIN_PLANE,
    0, 0, 0, 0
};


// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static void WindowEnd(void) {
    if (resource_context_handle_) {
        wglMakeCurrent(0, 0);
        wglDeleteContext(resource_context_handle_);
    }

    if (window_handle_) {
        if (device_context_handle_) ReleaseDC(window_handle_, device_context_handle_);
        DestroyWindow(window_handle_);
    }

    UnregisterClass(kWindowClassName, instance_handle_);

#ifdef FULLSCREEN
    ShowCursor(1);
#endif
}


int APIENTRY wWinMain(_In_ HINSTANCE instance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Perform application initialization:
    if (!InitInstance(instance, nCmdShow, false, 0)) {
        return FALSE;
    }

    // NEHE MULTISAMPLE
    bool arbMultisampleSupported = false;
    int arbMultisampleFormat = 0;
    // Get Our Pixel Format
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    HDC hDC = device_context_handle_;
    int pixelFormat;
    int valid;
    UINT numFormats;
    float fAttributes[] = { 0, 0 };
    const int kMaxNumMultisamples = 16;
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
        WGL_DEPTH_BITS_ARB, 0,
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
        WindowEnd();
        // Remove all messages...
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat)) {
            return FALSE;
        }
    }
    glEnable(GL_MULTISAMPLE_ARB);

    // Set transformation matrix to do aspect ratio adjustment
    RECT window_rectangle;
    GetWindowRect(window_handle_, &window_rectangle);
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

    // The main container holding all the thread information
    ThreadInformation thread_information;
    // Debug: Add some data
    thread_information.AddThread(-0.6f, 0.4f, 2, 0.5f, -0.1f, 52);

    // Main message loop:
    long start_time_ = timeGetTime();
    bool done = false;

    MSG msg;
    while (!done) {
        long t = timeGetTime() - start_time_;

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.7f, 0.3f);
        glVertex3f(0.0f, 0.7f, 0.0f);
        glColor3f(0.7f, 1.0f, 0.3f);
        glVertex3f(-0.7f, -0.7f, 0.0f);
        glColor3f(0.3f, 0.7f, 1.0f);
        glVertex3f(0.7f, -0.7f, 0.0f);
        glEnd();
#endif
        thread_information.Draw(1.0f, 0.7f, 0.3f, 0.1f);

        SwapBuffers(device_context_handle_);
    }

    // Cleanup
    WindowEnd();

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
                  int custom_pixel_format) {
    instance_handle_ = instance; // Store instance handle in our global variable
    DWORD window_style;
    RECT window_rectangle;
    unsigned int	pixel_format;

    WNDCLASSA window_class;

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WndProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = kWindowClassName;

    RegisterClassA(&window_class);

    // Set default window size (if not fullscreen)
    window_rectangle.left   = 0;
    window_rectangle.top    = 0;
    window_rectangle.right  = XRES;
    window_rectangle.bottom = YRES;

#ifdef FULLSCREEN
    window_style = WS_VISIBLE | WS_POPUP | WS_MAXIMIZE;  // | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    ShowCursor(0);
#else
    window_style   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    AdjustWindowRect(&window_rectangle, window_style, 0);
#endif

    window_handle_ = CreateWindowA(kWindowClassName, "OpenGL 1 sample", window_style,
        (GetSystemMetrics(SM_CXSCREEN) - window_rectangle.right + window_rectangle.left) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - window_rectangle.bottom + window_rectangle.top) / 2,
        window_rectangle.right - window_rectangle.left,
        window_rectangle.bottom - window_rectangle.top,
        0,  // Parent window
        0,  // Menu
        instance_handle_,
        0);  // lParam
    if (!window_handle_) return false;

    if (!(device_context_handle_ = GetDC(window_handle_))) return false;

    if (!use_custom_pixel_format) {
        if (!(pixel_format = ChoosePixelFormat(device_context_handle_, &kPixelFormatDescriptor))) {
            return false;
        }
        if (!SetPixelFormat(device_context_handle_, pixel_format, &kPixelFormatDescriptor)) {
            return false;
        }
    } else {
        if (!SetPixelFormat(device_context_handle_, custom_pixel_format, &kPixelFormatDescriptor)) {
            return false;
        }
    }

#if 0
    // create attribute context
    HGLRC temp_opengl_context;
    if (!(temp_opengl_context = wglCreateContext(device_context_handle_))) return false;
    if (!wglMakeCurrent(device_context_handle_, temp_opengl_context)) return false;
    if (!(resource_context_handle_ = wglCreateContextAttribsARB(device_context_handle_, NULL, glAttribs))) return 0;
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(temp_opengl_context);
#else
    // create basic context
    if (!(resource_context_handle_ = wglCreateContext(device_context_handle_))) return false;
#endif
    if (!wglMakeCurrent(device_context_handle_, resource_context_handle_)) return false;

    // Create openGL functions
#if 0
    for (int i=0; i<NUM_GL_NAMES; i++) glFP[i] = (GenFP)wglGetProcAddress(glnames[i]);
#endif

#if 0
    // Test GL version
    int major_version = -1;
    int minor_version = -1;
    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    printf("Reported GL Version %d.%d [Supported]\n", major_version, minor_version);
#endif

    ShowWindow(window_handle_, nCmdShow);
    UpdateWindow(window_handle_);

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

    if (message == WM_CHAR) {
        if (wParam == VK_ESCAPE ) {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
