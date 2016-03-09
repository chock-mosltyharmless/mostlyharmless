#include "stdafx.h"
#include "Karaoke.h"
#include "chockNgt.h"
#include "Configuration.h"


Karaoke::Karaoke()
{
    ToBeginning();
}


Karaoke::~Karaoke()
{
}

void Karaoke::ToBeginning(void) {
    draw_kenchiro_ = false;
    kenchiro_start_time_ = last_call_time_;
    has_white_fade_ = false;
    to_white_ = 1.0f;
    kenchiro_id_ = 0;
    scene_ = TRENNUNG;
}

int Karaoke::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;    

    float kFrameSkipTimes[3] = {0.0f, 0.0f, 0.0f};
    float kFrameOpenTimes[3] = {0.0f, 0.0f, 0.0f};
    float kFrameCloseTimes[3] = {749.0f, 64.0f, 156.5f};
    // 0: Clock
    // 1: Center frame
    int kVideoPosition[3] = {1, 1, 0};
    const char *kKenchiroVideos[3] = {
        "Naka_Kneipe_02.wmv",
        "Naka_Udagawa_01.wmv",
        "S22_fight02_hell.wmv",
    };

    float video_time = time - kenchiro_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    bool start_kenchiro = false;
    float kFrameOpenTime = kFrameOpenTimes[kenchiro_id_];
    float kFrameCloseTime = kFrameCloseTimes[kenchiro_id_] - kFrameSkipTimes[kenchiro_id_];
    if (draw_kenchiro_) {
        if (video_time > kFrameOpenTime) {
            start_kenchiro = true;
        }
    }    

    if (has_white_fade_) {
        to_white_ += (time - last_call_time_) * 1.0f;
        if (to_white_ > 1.75f) {
            to_white_ = 2.0f;
            is_scene_finished = true;
        }
    } else {
        to_white_ -= time - last_call_time_;
        if (to_white_ < 0.0f) to_white_ = 0.0f;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID("kneipe_drawings.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Kenshiro behind the clock
    if (draw_kenchiro_ && kVideoPosition[kenchiro_id_] == 0) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id,
            error_string, video_time + 0.5f + kFrameSkipTimes[kenchiro_id_]) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.665f, -0.46f, 0.814f, 0.476f, 0.3f, 0.65f, 0.25f, 0.8f, 1.0f);
    }

    // Kenchiro in main frame
    if (draw_kenchiro_ && kVideoPosition[kenchiro_id_] == 1) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id,
            error_string, video_time + 0.5f + kFrameSkipTimes[kenchiro_id_]) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.3f, 0.339f, 0.7f, -0.1, 1.0f, 0.0f, 0.1f, 0.9f, 1.0f);
        if (video_time > kFrameCloseTime) draw_kenchiro_ = false;
    }

    // Room
    texture_name = "kneipe_room_notv.png";
    if (start_kenchiro && kVideoPosition[kenchiro_id_] == 0) texture_name = "kneipe_room_noclock.png";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Night lighting of the room
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("kneipe_lighting.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Fade to white
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuadColor(-1.0f, 1.0f, 1.0f, -1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, to_white_);

    // Darkening borders
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("vignette.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    float glow_y = 0.645f;
    float glow_x = -0.665f + cosf(0.0f)*0.1025f;

    // Draw opened lid
    if (start_kenchiro && kVideoPosition == 0) {
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
        texture_name = "kneipe_clock.png";
        if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float brightness = 1.0f;
        DrawQuadColor(-0.665f, -0.665f + cosf(open_rad)*0.205f, 0.894f, 0.476f,
            brightness, brightness, brightness, 1.0f);
        glow_x = -0.665f + cosf(open_rad)*0.1025f;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    texture_name = "kneipe_clock_glow.png";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    float brightness = 1.0f;
    DrawQuad(glow_x - 0.4f, glow_x + 0.4f, glow_y + 0.4f, glow_y - 0.4f, 0.2f - 0.2f * to_white_);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    last_call_time_ = time;

    if (is_scene_finished) return 1;  // No fade-out so far.
    return 0;  // no error
}

void Karaoke::StartKenchiro(void) {
    draw_kenchiro_ = true;
    kenchiro_start_time_ = last_call_time_;

    switch (scene_) {
    case TRENNUNG:
        kenchiro_id_ = 0;
        PlaySound("textures/Naka_Kneipe_02.wav", NULL, SND_ASYNC);
        break;
    case BAHNHOF_BAR:
        kenchiro_id_ = 1;
        PlaySound("textures/Naka_Udagawa_01.wav", NULL, SND_ASYNC);
        break;
    case MITARBEITER:
    case SEKUHARA:
        kenchiro_id_ = 2;
        PlaySound("textures/S22_fight02_nr_nomisa_skip0.wav", NULL, SND_ASYNC);
    }
}

void Karaoke::EndKenchiro(void) {
    draw_kenchiro_ = false;
    PlaySound("textures/silence.wav", NULL, SND_ASYNC);
}

