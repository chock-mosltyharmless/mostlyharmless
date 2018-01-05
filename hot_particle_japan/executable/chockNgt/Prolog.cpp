#include "stdafx.h"
#include "Prolog.h"
#include "chockNgt.h"
#include "Configuration.h"

Prolog::Prolog()
{
    ToBeginning();
}


Prolog::~Prolog()
{
}

void Prolog::ToBeginning(void) {
    video_start_time_ = last_call_time_;
    show_video_ = false;
    brightness_ = 0.0f;
    has_light_ = false;
}

void Prolog::StartVideo(void) {
    show_video_ = true;
    video_start_time_ = last_call_time_;
}

int Prolog::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    bool is_scene_finished = false;

    // Adjust brightness according to time difference
    if (has_light_) {
        brightness_ += (time - last_call_time_) * 2.0f;
        if (brightness_ > 1.0f) brightness_ = 1.0f;
    } else {
        brightness_ -= (time - last_call_time_) * 2.5f;
        if (brightness_ < 0.0f) {
            brightness_ = 0.0f;
            is_scene_finished = true;
        }
    }

    last_call_time_ = time;
    float video_time = time - video_start_time_;

    float kVideoEndTime = 52.0f;

    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha

    if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuadColor(-1.0f, 1.0f, -1.0f, 1.0f, brightness_, brightness_*0.97f, brightness_*0.85f, 1.0f);

    if (show_video_) {
        float alpha = video_time * 0.5f;
        if (alpha > 1.0f) {
            StartLight();
            alpha = 1.0f;
        }
        if (video_time > kVideoEndTime) {
            alpha = 1.0f - video_time + kVideoEndTime;
            if (alpha < 0.0f) {
                alpha = 0.0f;
                EndVideo();
            }
        }
        if (textureManager.getVideoID("Fukushima-Fahrt_small.wmv", &tex_id, error_string, video_time) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, alpha);
    }

    // Darkening borders
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("vignette.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f);

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha
    
    if (is_scene_finished) return 1;
    return 0;
}
