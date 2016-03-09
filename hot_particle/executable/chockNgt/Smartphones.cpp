#include "stdafx.h"
#include "Smartphones.h"
#include "chockNgt.h"
#include "Configuration.h"

Smartphones::Smartphones()
{
    ToBeginning();
}


Smartphones::~Smartphones()
{
}

void Smartphones::ToBeginning(void) {
    next_picture_id_ = kNumCowPictures - 1;
    last_picture_take_time_ = last_call_time_;
    show_cows_ = false;
    has_flashed_ = false;
}

void Smartphones::TakeNextPicture(void) {
    show_cows_ = true;
    next_picture_id_++;
    if (next_picture_id_ >= kNumCowPictures) next_picture_id_ = 0;
    last_picture_take_time_ = last_call_time_;
    has_flashed_ = false;
}

void Smartphones::NoMorePictures(void) {
    show_cows_ = false;
}


int Smartphones::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;

    const char *kCowTextures[kNumCowPictures] = {
        "cows_1.png",
        "cows_2.png",
        "cows_3.png",
        "cows_4.png",
        "cows_5.png",
        "cows_6.png",
        "cows_7.png",
        "cows_8.png",
    };
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID("smartphones_drawings_one.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Draw the cow picture
    if (show_cows_) {
        float picture_time = time - last_picture_take_time_;
        float x_shift = 0.0f;
        float y_shift = 0.0f;
        const float kSeekTime = 0.5f;
        const float kFlashTime = 0.2f;
        if (picture_time < kSeekTime) {
            x_shift = (1.0f - picture_time / kSeekTime) * 0.025f * sinf(time * 2.3f) + 0.01f * cosf(time * 7.3f) - 0.005f * cosf(time * 23.5f);
            y_shift = (1.0f - picture_time / kSeekTime) * 0.025f * cosf(time * 3.1f) - 0.01f * sinf(time * 8.1f) + 0.005f * sinf(time * 27.4f);
        }
        if (textureManager.getTextureID(kCowTextures[next_picture_id_], &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float border_size = 0.04f;
        DrawQuad(-0.285f - border_size + x_shift, 0.28f + border_size + x_shift,
                 0.762f + border_size + y_shift, -0.052f - border_size + y_shift,
                 1.0f);

        float white_alpha = 0.0f;
        if (picture_time >= kSeekTime) {
            white_alpha = 1.0f - (picture_time - kSeekTime) / kFlashTime;
            if (!has_flashed_) {
                PlaySound("textures/flash.wav", NULL, SND_ASYNC);
                has_flashed_ = true;
            }
        }
        if (white_alpha < 0.0f) white_alpha = 0.0f;
        if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.285f, 0.28f, 0.762f, -0.052f, white_alpha);
    }

    // Draw the phones (exclude bottom):
    if (textureManager.getTextureID("smartphones_room_nobottom.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Darkening borders
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("vignette.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    last_call_time_ = time;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return 1;  // no fade-out so far.
    return 0;
}