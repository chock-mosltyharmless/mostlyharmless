// OpenGL_1_sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Configuration.h"

#include <algorithm>

// Gathered calibration data
#include "PictureWriter.h"
#include "Camera.h"

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
Camera camera_;

// Forward declarations of functions included in this code module:
bool InitInstance(HINSTANCE instance, int nCmdShow,
                  bool use_custom_pixel_format,
                  int custom_pixel_format,
                  LPCSTR window_class_name,
                  WindowHandles *handles,
                  const char *window_title);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static void RemoveNoise(int (*data)[2], int size[2], int dimension, int window_radius)
{
    const int kWindowSize = window_radius * 2 + 1;
    int *window = new int[kWindowSize];
    int *tmp_data = new int[size[0] * size[1]];
    int x_stride = 1;
    int y_stride = 1;
    if (dimension == 0) y_stride = size[0];
    if (dimension == 1) x_stride = size[0];

    // Copy to tmpdata
    for (int i = 0; i < size[0] * size[1]; i++) tmp_data[i] = data[i][dimension];

    for (int y = 0; y < size[1 - dimension]; y++)
    {
        int y_pos = y_stride * y;
        for (int x = window_radius; x < size[dimension] - window_radius; x++)
        {
            int start_pos = y_pos + x_stride * x;
            for (int i = -window_radius; i <= window_radius; i++)
            {
                window[i + window_radius] = tmp_data[start_pos + i * x_stride];
            }
            std::sort(window, window + kWindowSize);
            
            data[start_pos][dimension] = window[window_radius];  // simple median smoothing
        }
    }

    delete [] tmp_data;
    delete [] window;
}

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
        if (!InitInstance(instance, nCmdShow, true, arbMultisampleFormat, kCameraWindowClassName, &camera_window_, "For debugging")) {
            return FALSE;
        }
    } else {
        if (!InitInstance(instance, nCmdShow, false, 0, kCameraWindowClassName, &camera_window_, "For debugging")) {
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

    if (camera_.Init() < 0) return -1;

    MSG msg;

    // Do the start delay
    const int kCalibrationDelay = CALIBRATION_DELAY;
    while (timeGetTime() - start_time < kCalibrationDelay && !done)
    {
        camera_.GetFrame(true);  // Force camera to turn on...
        Sleep(100);
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(main_window_.device_context_handle_);
    }

    start_time = timeGetTime();

#ifdef CALIBRATE_CAMERA
    // Calibrate background black
    while (GetFrame(true) <= 0)
    {
        Sleep(10);
        SwapBuffers(main_window_.device_context_handle_);
    }
    for (int frame = 0; frame < NUM_ACCUMULATE;)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(main_window_.device_context_handle_);
        frame += GetFrame(false);
    }
    camera_.CalibrateCameraToBlack();
#endif
    
    // Reset the time for accumulate
    last_flash = timeGetTime() - start_time;

#ifndef LATENCY_MEASUREMENT
    int(*camera_to_projector)[2] = new int[camera_.width() * camera_.height()][2];
    for (int i = 0; i < camera_.width() * camera_.height(); i++)
    {
        camera_to_projector[i][0] = 0;
        camera_to_projector[i][1] = 0;
    }
#endif

    while (!done) {
        long cur_time = timeGetTime() - start_time;
        float time_seconds = 0.001f * static_cast<float>(cur_time);

        bool refresh_accumulate = true;  // Flag that deletes accumulate buffer in camera
        bool check_calibration = false;  // Use accumulate buffer to check

        int num_processed_frames = 1;

#ifdef LATENCY_MEASUREMENT
        refresh_accumulate = true;  // No accumulation for latency measurements
#else
        if (cur_time - last_flash > USE_LATENCY)
        {
            refresh_accumulate = false;
        }
#endif

        num_processed_frames = camera_.GetFrame(refresh_accumulate);  // Check return value?

#ifndef LATENCY_MEASUREMENT
        if (cur_time - last_flash > USE_LATENCY)
        {
            num_accumulated += num_processed_frames;
            if (num_accumulated >= NUM_ACCUMULATE)
            {
                num_accumulated = 0;
                check_calibration = true;
            }
        }
#endif

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Draw main window:
        if (!wglMakeCurrent(main_window_.device_context_handle_, main_window_.resource_context_handle_)) return -1;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        
        // Flash for latency testing
#ifdef LATENCY_MEASUREMENT
        const int kWaitFlashTime = 2000;
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

        glLoadIdentity();  // Reset The View

#ifndef LATENCY_MEASUREMENT
        static bool do_row_calibration = false;

        if (do_row_calibration)  // Check for second pass (row calibration)
        {
            if (check_calibration)
            {
                // This is the last time that the column is shown - check it
                camera_.NormalizeAccumulateBuffer();
                // Apply data to calibration
                int add_amount = 1 << (CALIBRATION_LOG_X_RESOLUTION - index - 1);
                for (int i = 0; i < camera_.width() * camera_.height(); i++)
                {
                    if (camera_.accumulate_buffer_[i * 4 + 3] > CALIBRATION_THRESHOLD)
                    {
                        camera_to_projector[i][0] += add_amount;
                    }
                }

                // Debugging: Save calibration image
                char filename[1024];
                sprintf(filename, "pictures/rows.%d.prefilter.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector, filename, CALIBRATION_X_RESOLUTION);

                int size[2] = {camera_.width(), camera_.height()};
                int window_radius = CALIBRATION_LOG_X_RESOLUTION - index + 1;
                if (window_radius > 3) window_radius = 3;
                RemoveNoise(camera_to_projector, size, 0, window_radius);

                // Debugging: Save calibration image
                sprintf(filename, "pictures/rows.%d.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector, filename, CALIBRATION_X_RESOLUTION);

                index++;
                // All columns processed, go to row mode
                if (index >= CALIBRATION_LOG_X_RESOLUTION) done = true;

                last_flash = timeGetTime() - start_time;
            }

            // show image (for calibration)
            int num_bars = 1 << index;
            float bar_width = 1.0f / (float)num_bars;

            for (int bar = 0; bar < num_bars; bar++)
            {
                float x_pos = bar_width * (1 + 2 * bar) - 1.0f;
                glColor3f(1.0f, 1.0f, 1.0f);
                glRectf(x_pos, -1.0f, x_pos + bar_width, 1.0f);
            }
        }
        else
        {
            // Do first pass (columns)
            if (check_calibration)
            {
                // This is the last time that the column is shown - check it
                camera_.NormalizeAccumulateBuffer();
                // Apply data to calibration
                for (int i = 0; i < camera_.width() * camera_.height(); i++)
                {
                    if (camera_.accumulate_buffer_[i * 4 + 3] > CALIBRATION_THRESHOLD)
                    {
                        camera_to_projector[i][1] += 1 << (CALIBRATION_LOG_Y_RESOLUTION - index - 1);
                    }
                }

                // Debugging: Save calibration image
                char filename[1024];
                sprintf(filename, "pictures/columns.%d.prefilter.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector, filename, CALIBRATION_X_RESOLUTION);

                int size[2] = { camera_.width(), camera_.height() };
                int window_radius = CALIBRATION_LOG_Y_RESOLUTION - index + 1;
                if (window_radius > 3) window_radius = 3;
                RemoveNoise(camera_to_projector, size, 1, window_radius);

                // Debugging: Save calibration image
                sprintf(filename, "pictures/columns.%d.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector, filename, CALIBRATION_X_RESOLUTION);

                index++;
                // All columns processed, go to row mode
                if (index >= CALIBRATION_LOG_Y_RESOLUTION)
                {
                    do_row_calibration = true;
                    index = 0;
                }

                last_flash = timeGetTime() - start_time;
            }

            // show image (for calibration)
            int num_bars = 1 << index;
            float bar_height = 1.0f / (float)num_bars;

            for (int bar = 0; bar < num_bars; bar++)
            {
                float y_pos = bar_height * (1 + 2 * bar) - 1.0f;
                glColor3f(1.0f, 1.0f, 1.0f);
                glRectf(-1.0f, y_pos, 1.0f, y_pos + bar_height);
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

        camera_.SetTexture();

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