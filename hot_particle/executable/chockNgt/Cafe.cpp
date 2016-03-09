#include "stdafx.h"
#include "Cafe.h"
#include "chockNgt.h"
#include "Configuration.h"


Cafe::Cafe()
{
    ToBeginning();
}


Cafe::~Cafe()
{
}

void Cafe::ToBeginning(void) {
    has_white_fade_ = false;
    to_white_ = 1.0f;
}

int Cafe::Draw(float time) {
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID("cafe_drawings.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Room
    texture_name = "cafe_room.png";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Night lighting of the room
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("cafe_lighting.png", &tex_id, error_string)) {
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

