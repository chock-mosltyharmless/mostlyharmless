#include "stdafx.h"
#include "Car.h"
#include "chockNgt.h"
#include "Configuration.h"

Car::Car()
{
    ToBeginning();
}


Car::~Car()
{
}

void Car::ToBeginning(void) {
    has_white_fade_ = false;
    to_white_ = 1.0f;
    draw_video_ = false;
    video_start_time_ = last_call_time_;
    scene_ = BEGRUSSUNG;
}

int Car::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;    

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

    float video_time = time - video_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Room
    texture_name = "car_room.png";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    last_call_time_ = time;

    if (is_scene_finished) return 1;  // No fade-out so far.
    return 0;  // no error
}



