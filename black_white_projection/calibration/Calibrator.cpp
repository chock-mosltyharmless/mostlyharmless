#include "stdafx.h"
#include "Calibrator.h"

#include <algorithm>

#include "Configuration.h"
#include "PictureWriter.h"

static void RemoveNoise(int(*data)[2], int size[2], int dimension, int window_radius)
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

    delete[] tmp_data;
    delete[] window;
}

Calibrator::Calibrator()
{
}

Calibrator::~Calibrator()
{
    if (project_buffer_) delete [] project_buffer_;
}

bool Calibrator::Init(HDC main_window_device_handle, HGLRC main_window_resource_handle,
                      HDC debug_window_device_handle, HGLRC debug_window_resource_handle)
{
    main_window_device_handle_ = main_window_device_handle;
    main_window_resource_handle_ = main_window_resource_handle;
    debug_window_device_handle_ = debug_window_device_handle;
    debug_window_resource_handle_ = debug_window_resource_handle;
    
    long start_time = timeGetTime();

    if (camera_.Init() < 0) return false;

    MSG msg;

    // Do the start delay
    const int kCalibrationDelay = CALIBRATION_DELAY;
    while (timeGetTime() - start_time < kCalibrationDelay)
    {
        Sleep(10);
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            bool quit = false;
            if (msg.message == WM_QUIT) quit = true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (quit) return false;
        }
        
        if (!wglMakeCurrent(main_window_device_handle, main_window_resource_handle)) return false;
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(main_window_device_handle);

        // Force camera to turn on and show debug window
        for (int i = 0; i < NUM_ACCUMULATE; i++)
        {
            while (camera_.GetFrame(i == 0) <= 0) Sleep(10);
        }
        camera_.NormalizeAccumulateBuffer();  // So that the debug display can be shown
        DrawCameraDebug(debug_window_device_handle, debug_window_resource_handle);
    }

    start_time = timeGetTime();

#ifdef CALIBRATE_CAMERA
    // Calibrate background black
    while (GetFrame(true) <= 0)
    {
        Sleep(10);
        SwapBuffers(main_window_device_handle);
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
        SwapBuffers(main_window_device_handle);
        frame += GetFrame(false);
    }
    camera_.CalibrateCameraToBlack();
#endif

    // Initialize the texture used for projection
    if (!wglMakeCurrent(main_window_device_handle, main_window_resource_handle)) return false;
    project_buffer_ = new unsigned char[CALIBRATION_Y_RESOLUTION * CALIBRATION_X_RESOLUTION][4];
    memset(project_buffer_, 0, CALIBRATION_X_RESOLUTION * CALIBRATION_Y_RESOLUTION * 4);

    glEnable(GL_TEXTURE_2D);						                    // Enable Texture Mapping
    glGenTextures(1, &(texture_id_));
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtering
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 CALIBRATION_X_RESOLUTION, CALIBRATION_Y_RESOLUTION,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, project_buffer_);
    if (!wglMakeCurrent(debug_window_device_handle, debug_window_resource_handle)) return false;

    return true;
}

bool Calibrator::Calibrate()
{
    camera_to_projector_ = new int[camera_.width() * camera_.height()][2];
    for (int i = 0; i < camera_.width() * camera_.height(); i++)
    {
        camera_to_projector_[i][0] = 0;
        camera_to_projector_[i][1] = 0;
    }

    MSG msg;
    int num_accumulated = 0;
    int index = 0;  // Index of currently handled point/line
    bool done = false;
    long start_time = timeGetTime();

    // Reset the time for accumulate
    long last_flash = 0;

    while (!done)
    {
        long cur_time = timeGetTime() - start_time;
        float time_seconds = 0.001f * static_cast<float>(cur_time);

        bool refresh_accumulate = true;  // Flag that deletes accumulate buffer in camera
        bool check_calibration = false;  // Use accumulate buffer to check

        int num_processed_frames = 1;

        if (cur_time - last_flash > USE_LATENCY)
        {
            refresh_accumulate = false;
        }

        num_processed_frames = camera_.GetFrame(refresh_accumulate);  // Check return value?

        if (cur_time - last_flash > USE_LATENCY)
        {
            num_accumulated += num_processed_frames;
            if (num_accumulated >= NUM_ACCUMULATE)
            {
                num_accumulated = 0;
                check_calibration = true;
            }
        }

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Draw main window:
        if (!wglMakeCurrent(main_window_device_handle_, main_window_resource_handle_)) return false;
        
        glDisable(GL_TEXTURE_2D);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glLoadIdentity();  // Reset The View

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
                        camera_to_projector_[i][0] += add_amount;
                    }
                }

                // Debugging: Save calibration image
                char filename[1024];
                sprintf(filename, "pictures/rows.%d.prefilter.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector_, filename, CALIBRATION_X_RESOLUTION);

                int size[2] = { camera_.width(), camera_.height() };
                int window_radius = CALIBRATION_LOG_X_RESOLUTION - index + 1;
                if (window_radius > 3) window_radius = 3;
                RemoveNoise(camera_to_projector_, size, 0, window_radius);

                // Debugging: Save calibration image
                sprintf(filename, "pictures/rows.%d.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector_, filename, CALIBRATION_X_RESOLUTION);

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
                        camera_to_projector_[i][1] += 1 << (CALIBRATION_LOG_Y_RESOLUTION - index - 1);
                    }
                }

                // Debugging: Save calibration image
                char filename[1024];
                sprintf(filename, "pictures/columns.%d.prefilter.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector_, filename, CALIBRATION_X_RESOLUTION);

                int size[2] = { camera_.width(), camera_.height() };
                int window_radius = CALIBRATION_LOG_Y_RESOLUTION - index + 1;
                if (window_radius > 3) window_radius = 3;
                RemoveNoise(camera_to_projector_, size, 1, window_radius);

                // Debugging: Save calibration image
                sprintf(filename, "pictures/columns.%d.tga", index);
                PictureWriter::SaveTGA(camera_.width(), camera_.height(), camera_to_projector_, filename, CALIBRATION_X_RESOLUTION);

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

        SwapBuffers(main_window_device_handle_);

        DrawCameraDebug(debug_window_device_handle_, debug_window_resource_handle_);
    }

    return true;
}

static void DrawQuad(void)
{
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
}

void Calibrator::ShowConstColor(unsigned char red, unsigned char green, unsigned char blue)
{
    // This is incorrect: Just use the color
    for (int y = 1; y < CALIBRATION_Y_RESOLUTION - 1; y++)
    {
        for (int x = 1; x < CALIBRATION_X_RESOLUTION - 1; x++)
        {
            int index = y * CALIBRATION_X_RESOLUTION + x;
            project_buffer_[index][0] = blue;
            project_buffer_[index][1] = green;
            project_buffer_[index][2] = red;
            project_buffer_[index][3] = 255;
        }
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CALIBRATION_X_RESOLUTION, CALIBRATION_Y_RESOLUTION,
                     GL_BGRA, GL_UNSIGNED_BYTE, project_buffer_);

    // Rendering of the camera rect
    glLoadIdentity();  // Reset The View
    DrawQuad();
}

void Calibrator::DrawCameraDebug(HDC debug_window_device_handle, HGLRC debug_window_resource_handle)
{
    // Render camera window
    if (!wglMakeCurrent(debug_window_device_handle, debug_window_resource_handle)) return;

    // Set transformation matrix to do aspect ratio adjustment
    glLoadIdentity();  // Reset The View

    camera_.SetTexture();

    DrawQuad();
    SwapBuffers(debug_window_device_handle);
}
