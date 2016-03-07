#include "stdafx.h"
#include "Zimmer.h"
#include "chockNgt.h"
#include "Configuration.h"

Zimmer::Zimmer()
{
    ToBeginning();
}


Zimmer::~Zimmer()
{
}

void Zimmer::ToBeginning(void) {
    draw_kenchiro_ = false;
    kenchiro_id_ = 0;
    kenchiro_start_time_ = last_call_time_;
    has_light_ = false;
    brightness_ = 0.0f;
}

int Zimmer::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;

    float kFrameSkipTimes[6] = {6.0f, 7.0f, 5.5f, 14.0f, 7.0f, 6.5f};
    float kFrameOpenTimes[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    float kFrameCloseTimes[6] = {152.5f, 100.5f, 305.0f, 85.0f, 73.0f, 313.0f};
    const char *kRoomTextures[6] = {
        "zuhause_room_maerz_two.png",
        "zuhause_room_maerz_two.png",
        "zuhause_room_maerz_two.png",
        "zuhause_room_maerz_two.png",
        "proberaum_room.png",
        "zuhause_room_maerz_two.png"
    };
    const char *kRoomNoClockTextures[6] = {
        "zuhause_room_maerz_noclock.png",
        "zuhause_room_maerz_noclock.png",
        "zuhause_room_maerz_noclock.png",
        "zuhause_room_maerz_noclock.png",
        "proberaum_room_noclock.png",
        "zuhause_room_maerz_noclock.png"
    };
    const char *kKenchiroVideos[6] = {
        "S1_wachauf02_hell.wmv",
        "S20_Kitchomu_hell.wmv",
        "S23_Picasso03_hell.wmv",
        "S27_schwimmen03_hell.wmv",
        "S28_wasma02_hell.wmv",
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
    if (textureManager.getTextureID("zuhause_drawings_maerz_night.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Kenshiro behind the clock
    if (draw_kenchiro_) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id, error_string,
                                      video_time + 0.5f + kFrameSkipTimes[kenchiro_id_]) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.665f, -0.46f, 0.814f, 0.476f, 0.3f, 0.65f, 0.25f, 0.8f, 1.0f);
    }

    // Room
    texture_name = kRoomTextures[kenchiro_id_];
    if (start_kenchiro) texture_name = kRoomNoClockTextures[kenchiro_id_];
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Night lighting of the room
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("zuahuse_lighting.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Darkening borders
    if (textureManager.getTextureID("vignette.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Draw opened lid
    if (start_kenchiro) {
        float open_rad = 0.0f;
        float speed = 1.8f;
        float open_time = video_time - kFrameOpenTime;
        if (open_time * speed <= 1.0f) open_rad = sinf(open_time * speed * PIF * 0.5f);
        if (open_time * speed > 1.0f) open_rad = sinf(1.0f * PIF * 0.5f);
        speed = 1.2f;  // close time is slower
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
        texture_name = "zuhause_clock_two.png";
        if (open_rad > 0.5f * 3.1415f) texture_name = "zuhause_clock_blank.png";
        if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float brightness = 0.48f - sinf(open_rad)*0.2f;
        if (open_rad > 3.1415f * 0.5f) brightness = 0.4f + 0.2f * sinf(open_rad);
        DrawQuadColor(-0.665f, -0.665f + cosf(open_rad)*0.205f, 0.814f, 0.476f,
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