#include "stdafx.h"
#include "Zimmer.h"
#include "chockNgt.h"
#include "Configuration.h"

Zimmer::Zimmer()
{
}


Zimmer::~Zimmer()
{
}

int Zimmer::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;

    float kFrameOpenTimes[6] = {7.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    float kFrameCloseTimes[6] = {151.0f, 95.0f, 300.0f, 80.0f, 72.0f, 320.0f};
    const char *kKenchiroVideos[6] = {
        "S1_wachauf02.wmv",
        "S20_Kitchomu.wmv",
        "S23_Picasso03.wmv",
        "S27_schwimmen03.wmv",
        "S28_wasma02.wmv",
        "S29_Kobe02.wmv"
    };

    // Adjust brightness according to time difference
    if (has_light_) {
        brightness_ += (time - last_call_time_) * 2.0f;
        if (brightness_ > 1.0f) brightness_ = 1.0f;
    } else {
        brightness_ -= time - last_call_time_;
        if (brightness_ < 0.0f) brightness_ = 0.0f;
    }

    last_call_time_ = time;

    float video_time = time - kenchiro_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    bool start_kenchiro = false;
    float kFrameOpenTime = kFrameOpenTimes[kenchiro_id_];
    float kFrameCloseTime = kFrameCloseTimes[kenchiro_id_];
    if (draw_kenchiro_) {
        if (video_time > kFrameOpenTime) {
            start_kenchiro = true;
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID("zimmer_layer_1_night.tga", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f);

    // Kenshiro behind the clock
    if (draw_kenchiro_) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id, error_string, video_time) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        //DrawQuad(-0.7f, -0.4f, 0.7f, 0.35f, 0.3f, 0.7f, 0.25f, 0.8f, 1.0f);
        DrawQuad(-0.686f, -0.429f, 0.704f, 0.369f, 0.3f, 0.7f, 0.25f, 0.8f, 1.0f);
    }

    // Room
    texture_name = "zimmer_layer_2.tga";
    if (start_kenchiro) texture_name = "zimmer_layer_2_noclock.tga";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f);

    // Night lighting of the room
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("zimmer_layer_3.tga", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f);

    // Darkening borders
    if (textureManager.getTextureID("vignette.tga", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f);

    // Draw opened lid
    if (start_kenchiro) {
        float open_rad = 0.0f;
        float speed = 1.8f;
        float open_time = video_time - kFrameOpenTime;
        if (open_time * speed <= 1.0f) open_rad = sinf(open_time * speed * PIF * 0.5f);
        if (open_time * speed > 1.0f) open_rad = sinf(1.0f * PIF * 0.5f);
        speed = 0.8f;  // close time is slower
        float close_time = video_time - kFrameCloseTime;
        if (close_time > 0.0f && close_time * speed <= 1.0f) {
            open_rad = 1.0f - sinf(close_time * speed * PIF * 0.5f);
        }
        if (close_time * speed > 1.0f) {
            open_rad = 0.0f;
            EndKenchiro();
        }
        open_rad *= 0.75f * PIF;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        texture_name = "zimmer_layer_2_clock.tga";
        if (open_rad > 0.5f * 3.1415f) texture_name = "zimmer_layer_2_clock_back.tga";
        if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float brightness = 0.8f - sinf(open_rad)*0.3f;
        if (open_rad > 3.1415f * 0.5f) brightness = 0.5f + 0.4f * sinf(open_rad);
        DrawQuadColor(-0.685f, -0.685f + cosf(open_rad)*0.255f, 0.37f, 0.703f,
                      brightness, brightness, brightness, 1.0f);
    }

    // Dim the room
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DrawQuadColor(-1.0f, 1.0f, 1.0f, -1.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f - brightness_);

    return 0;  // no error
}

void Zimmer::StartKenchiro(int id) {
    draw_kenchiro_ = true;
    kenchiro_id_ = id;
    kenchiro_start_time_ = last_call_time_;
}

void Zimmer::EndKenchiro(void) {
    draw_kenchiro_ = false;
}