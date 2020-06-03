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
        Sleep(100);
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
        while (camera_.GetFrame(false) <= 0) Sleep(10);
        DrawCameraDebug(true);
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
    error_buffer_ = new unsigned int[CALIBRATION_Y_RESOLUTION * CALIBRATION_X_RESOLUTION][4];
    memset(error_buffer_, 0, CALIBRATION_X_RESOLUTION * CALIBRATION_Y_RESOLUTION * 4 * sizeof(int));
    back_buffer_ = new unsigned int[CALIBRATION_Y_RESOLUTION * CALIBRATION_X_RESOLUTION][4];
    memset(back_buffer_, 0, CALIBRATION_X_RESOLUTION * CALIBRATION_Y_RESOLUTION * 4 * sizeof(int));


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

        DrawCameraDebug(false);
    }

    return true;
}

static void DrawQuad(float flip_y = 0.0f)
{
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, flip_y);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(1.0f, flip_y);
    glVertex2f(+1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f - flip_y);
    glVertex2f(+1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f - flip_y);
    glVertex2f(+1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f - flip_y);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(0.0f, flip_y);
    glVertex2f(-1.0f, +1.0f);
    glEnd();
}

void Calibrator::MakeBlackAndWhite(unsigned char brightness)
{
    // Take a new frame from the camera
    camera_.GetFrame(false);

    // Delete the error buffer
    memset(error_buffer_, 0, CALIBRATION_X_RESOLUTION * CALIBRATION_Y_RESOLUTION * 4 * sizeof(int));

    // Accumulate error
    int height = camera_.height();
    int width = camera_.width();
    int camera_size = width * height;
    for (int i = 0; i < camera_size; i++)
    {
        int y = camera_to_projector_[i][1];
        int x = camera_to_projector_[i][0];

        int mean_color = 0;
        for (int c = 0; c < 3; c++)
        {
            mean_color += camera_.raw_data(i * 4 + c);
        }
        mean_color = (mean_color + 1) / 3;
        for (int c = 0; c < 3; c++)
        {
            int source_color = camera_.raw_data(i * 4 + c);
            int error = mean_color - source_color;
            error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c] += error;
        }
    }

    // Normalize error buffer to 0
    for (int y = 0; y < CALIBRATION_Y_RESOLUTION; y++)
    {
        for (int x = 0; x < CALIBRATION_X_RESOLUTION; x++)
        {
            int mean_error = 0;
            for (int c = 0; c < 3; c++)
            {
                mean_error += error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c];
            }
            mean_error = (mean_error + 1) / 3;
            for (int c = 0; c < 3; c++)
            {
                error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c] -= mean_error;
            }
        }
    }

    // propagate error to neighbor cells
    for (int y = 1; y < CALIBRATION_Y_RESOLUTION - 1; y++)
    {
        for (int x = 1; x < CALIBRATION_Y_RESOLUTION - 1; x++)
        {
            for (int c = 0; c < 3; c++)
            {
                int error = error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c];
                int diffuse = error / 6;
                error_buffer_[(y - 1) * CALIBRATION_X_RESOLUTION + x][c] += diffuse;
                error_buffer_[(y + 1) * CALIBRATION_X_RESOLUTION + x][c] += diffuse;
                error_buffer_[y * CALIBRATION_X_RESOLUTION + x - 1][c] += diffuse;
                error_buffer_[y * CALIBRATION_X_RESOLUTION + x + 1][c] += diffuse;
                error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c] -= 4 * diffuse;
            }
        }
    }

    const int kUpdateRate = 50;  // (50 / 65536)
    int new_color[3];
    for (int y = 1; y < CALIBRATION_Y_RESOLUTION - 1; y++)
    {
        for (int x = 1; x < CALIBRATION_X_RESOLUTION - 1; x++)
        {
            int index = y * CALIBRATION_X_RESOLUTION + x;

            for (int c = 0; c < 3; c++)
            {
                int orig_color = back_buffer_[index][c];
                new_color[c] = (kUpdateRate * error_buffer_[index][c]) + orig_color;
            }

            // Dirty brightness approximation
            int new_color_brightness =
                (new_color[0] + 4 * new_color[1] + 3 * new_color[2] + 4) / 8;
            int color_shift = 65536 * brightness - new_color_brightness;

            for (int c = 0; c < 3; c++)
            {
                new_color[c] += color_shift;
                if (new_color[c] < 0) new_color[c] = 0;
                if (new_color[c] > 255 * 65536) new_color[c] = 255 * 65536;
                back_buffer_[index][c] = new_color[c];
            }
        }
    }

    // Copy to texture buffer
    for (int i = 0; i < CALIBRATION_Y_RESOLUTION * CALIBRATION_X_RESOLUTION * 4; i++)
    {
        project_buffer_[0][i] = back_buffer_[0][i] >> 16;
    }

    // Display on main screen
    wglMakeCurrent(main_window_device_handle_, main_window_resource_handle_);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CALIBRATION_X_RESOLUTION, CALIBRATION_Y_RESOLUTION,
        GL_BGRA, GL_UNSIGNED_BYTE, project_buffer_);
    glLoadIdentity();  // Reset The View
    DrawQuad(1.0f);  // Flip y for some reason.
    SwapBuffers(main_window_device_handle_);

    DrawCameraDebug(true);
}

void Calibrator::ShowConstColor(unsigned char red, unsigned char green, unsigned char blue)
{
    // Take a new frame from the camera
    camera_.GetFrame(false);

    // Delete the error buffer
    memset(error_buffer_, 0, CALIBRATION_X_RESOLUTION * CALIBRATION_Y_RESOLUTION * 4 * sizeof(int));

    // Accumulate error
    int height = camera_.height();
    int width = camera_.width();
    int camera_size = width * height;
    int dest_color[3] = {blue, green, red};
    for (int i = 0; i < camera_size; i++)
    {
        int y = camera_to_projector_[i][1];
        int x = camera_to_projector_[i][0];

        for (int c = 0; c < 3; c++)
        {
            int source_color = camera_.raw_data(i * 4 + c);
            int error = dest_color[c] - source_color;
            error_buffer_[y * CALIBRATION_X_RESOLUTION + x][c] += error;
        }
    }

    // Probably I should low-pass filter the error_buffer_ so that pixel errors don't escalate.

    const int kUpdateRate = 50;  // (50 / 65536)
    for (int y = 1; y < CALIBRATION_Y_RESOLUTION - 1; y++)
    {
        for (int x = 1; x < CALIBRATION_X_RESOLUTION - 1; x++)
        {
            int index = y * CALIBRATION_X_RESOLUTION + x;
            
            for (int c = 0; c < 3; c++)
            {
                int orig_color = back_buffer_[index][c];
                int new_color = (kUpdateRate * error_buffer_[index][c]) + orig_color;
                if (new_color < 0) new_color = 0;
                if (new_color > 255 * 65536) new_color = 255 * 65536;
                back_buffer_[index][c] = new_color;
            }
        }
    }

    // Copy to texture buffer
    for (int i = 0; i < CALIBRATION_Y_RESOLUTION * CALIBRATION_X_RESOLUTION * 4; i++)
    {
        project_buffer_[0][i] = back_buffer_[0][i] >> 16;
    }

    // Display on main screen
    wglMakeCurrent(main_window_device_handle_, main_window_resource_handle_);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CALIBRATION_X_RESOLUTION, CALIBRATION_Y_RESOLUTION,
                     GL_BGRA, GL_UNSIGNED_BYTE, project_buffer_);
    glLoadIdentity();  // Reset The View
    DrawQuad(1.0f);  // Flip y for some reason.
    SwapBuffers(main_window_device_handle_);

    DrawCameraDebug(true);
}

void Calibrator::DrawCameraDebug(bool show_raw_data)
{
    // Render camera window
    if (!wglMakeCurrent(debug_window_device_handle_, debug_window_resource_handle_)) return;

    // Set transformation matrix to do aspect ratio adjustment
    glLoadIdentity();  // Reset The View

    camera_.SetTexture(show_raw_data);

    DrawQuad();
    SwapBuffers(debug_window_device_handle_);
}
