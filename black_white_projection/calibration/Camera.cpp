#include "stdafx.h"
#include "Camera.h"

#include <string>

#include "Configuration.h"
#include "MFUtility.h"  // for CHECK_HR

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

// Clip in range 0..255
static int clip(int x)
{
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
}

template<typename T> static inline void ImSwap(T& a, T& b) { T tmp = a; a = b; b = tmp; }
static inline float  ImFmod(float x, float y) { return fmodf(x, y); }

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
static void ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
static void ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}


Camera::Camera()
{
    video_reader_ = NULL;
    buffer_ = NULL;
    debug_buffer_ = NULL;
    accumulate_buffer_ = NULL;

    // Best guesses if camera is not calibrated
    reference_black_brightness_ = 0.0f;
    reference_white_brightness_ = 255.0f * (NUM_ACCUMULATE + 1);  // Always at least 2 are accumulated
}

Camera::~Camera()
{
    delete[] buffer_;
    buffer_ = NULL;
    delete[] debug_buffer_;
    debug_buffer_ = 0;
    delete accumulate_buffer_;
    accumulate_buffer_ = NULL;
    // TODO: Release video_reader_
}

int Camera::Init() {
    std::string intent = "WEBCAM_DEVICE_INDEX = " + std::to_string(WEBCAM_DEVICE_INDEX) + "\n";
    intent += "MEDIA_TYPE_INDEX = " + std::to_string(MEDIA_TYPE_INDEX) + "\n";
#ifdef LIST_DEVICES
    MessageBox(NULL, intent.c_str(), "Intent for Camera usage", MB_OK);
#endif

    IMFActivate **videoDevices = NULL;
    UINT32 videoDeviceCount = 0;
    IMFMediaSource *videoSource = NULL;
    IMFAttributes *videoConfig = NULL;
    IMFMediaType *videoSourceOutputType = NULL;
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
        &(video_reader_)), "Error creating video source reader.\n");

    // Show a list of all the subtypes
#ifdef LIST_DEVICES
    ListModes(video_reader_);
#endif

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

    // Get relevant information
    UINT64 value;
    CHECK_HR(pSrcOutMediaType->GetUINT64(MF_MT_FRAME_SIZE, &value), "Could not get video dimension");
    width_ = HI32(value);
    height_ = LO32(value);
    int texture_size = width_ * height_ * 4;
    buffer_ = new unsigned char[texture_size];
    debug_buffer_ = new unsigned char[texture_size];
    for (int i = 0; i < texture_size; i++) {
        buffer_[i] = 0;
        debug_buffer_[i] = 0;
    }

    // Create OpenGL texture suitable for the camera picture
    glGenTextures(1, &(texture_id_));
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtering
                                                                        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
                                                                        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        width(), height(),
        0, GL_BGRA, GL_UNSIGNED_BYTE, debug_buffer_);

    return 0;
}

// TODO: Change to median?
float Camera::GetAverageBrightness(void)
{
    // Calculate average brightness for later normalization
    int brightness_sum = 0;
    for (int i = 0; i < width_ * height_; i++)
    {
        brightness_sum += accumulate_buffer_[i * 4 + 3];
    }
    return ((float)brightness_sum) / (float)(width_ * height_);
}

// Called after all the data was accumulated
void Camera::NormalizeAccumulateBuffer(void)
{
    // Currently reference black and white are not used,
    // Everything is simply divided by NUM_ACCUMULATE...
    for (int i = 0; i < width_ * height_ * 4; i++)
    {
        accumulate_buffer_[i] =
            (int)(((float)accumulate_buffer_[i] - reference_black_brightness_)
                / reference_white_brightness_ * 255.0f);
    }

    // debug_buffer_ is now only used to debug stuff by showing it in the debug window.
    for (int i = 0; i < width_ * height_; i++)
    {
        int index = i * 4;
        for (int col = 0; col < 4; col++)
        {
            int out = accumulate_buffer_[index + col];
            if (out > 255) out = 255;
            debug_buffer_[index + col] = static_cast<unsigned char>(out);
        }
    }
}

void Camera::Update(bool refresh_accumulate)
{
    if (NULL == accumulate_buffer_)
    {
        accumulate_buffer_ = new int[width_ * height_ * 4];
        for (int i = 0; i < width_ * height_ * 4; i++) accumulate_buffer_[i] = 0;
    }

    for (int i = 0; i < width_ * height_ * 4; i++)
    {
        if (refresh_accumulate) accumulate_buffer_[i] = 0;
        int input = static_cast<int>(buffer_[i]);
        accumulate_buffer_[i] += input;
    }
}

bool Camera::DetectFlash()
{
    int count = 0;
    const float kFlashThreshold = 64;
    const int kCountThreshold = 5000;
    for (int i = 3; i < width_ * height_ * 4; i += 4) {
        if (buffer_[i] >= kFlashThreshold) {
            count++;
        }
    }
    return (count >= kCountThreshold);
}

int Camera::GetFrame(bool refresh_accumulate)
{
    IMFSample *videoSample = NULL;
    DWORD streamIndex, flags;
    LONGLONG llVideoTimeStamp;
    int result = 0;

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

        // This is for YUY2:
        // Convert to RGBA in camera_.buffer, A is brightness
        unsigned char *ptrOut = buffer_;
        byte *ptrIn = byteBuffer;
        byte *ptrColorIn = byteBuffer + width() * height();  // for NV12
        for (int y = 0; y < height(); y++) {
            for (int x = 0; x < width() / 2; x++) {
#ifdef MEDIA_SUBTYPE_NV12
                int y0 = ptrIn[(y * width() / 2 + x) * 2];
                int y1 = ptrIn[(y * width() / 2 + x) * 2 + 1];
                int u0 = ptrColorIn[(y / 2 * width() / 2 + x) * 2];
                int v0 = ptrColorIn[(y / 2 * width() / 2 + x) * 2 + 1];
#else YUY2
                int y0 = ptrIn[(y * width() / 2 + x) * 4];
                int u0 = ptrIn[(y * width() / 2 + x) * 4 + 1];
                int y1 = ptrIn[(y * width() / 2 + x) * 4 + 2];
                int v0 = ptrIn[(y * width() / 2 + x) * 4 + 3];
#endif
                int c = y0 - 16;
                int d = u0 - 128;
                int e = v0 - 128;
                ptrOut[0] = clip((298 * c + 516 * d + 128) >> 8); // blue
                ptrOut[1] = clip((298 * c - 100 * d - 208 * e + 128) >> 8); // green
                ptrOut[2] = clip((298 * c + 409 * e + 128) >> 8); // red
                ptrOut[3] = y0;
                c = y1 - 16;
                ptrOut[4] = clip((298 * c + 516 * d + 128) >> 8); // blue
                ptrOut[5] = clip((298 * c - 100 * d - 208 * e + 128) >> 8); // green
                ptrOut[6] = clip((298 * c + 409 * e + 128) >> 8); // red
                ptrOut[7] = y1;

                ptrOut += 8;
            }
        }

        // Update with background
        Update(refresh_accumulate);

        buf->Release();
        videoSample->Release();
        result = 1;
    }

    return result;
}

void Camera::SetTexture(bool show_raw_signal)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    const void *show_buffer = debug_buffer_;
    if (show_raw_signal) show_buffer = buffer_;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width(), height(),
        GL_BGRA, GL_UNSIGNED_BYTE, show_buffer);
}