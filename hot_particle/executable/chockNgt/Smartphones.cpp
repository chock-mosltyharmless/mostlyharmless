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
    for (int i = 0; i < 4; i++) {
        current_picture_id_[i] = kNumCowPictures - 1;
        last_picture_take_time_[i] = last_call_time_;
        show_pictures_[i] = false;
        has_flashed_[i] = false;
    }
    scene_ = SM_KUHE;
    has_white_fade_ = false;
    to_white_ = 0.0f;
    video_start_time_ = last_call_time_;
    current_panya_id_ = -1;
    panya_start_time_ = last_call_time_;
}

void Smartphones::TakeNextPicture(int position) {
    show_pictures_[position] = true;
    next_picture_id_++;
    if (next_picture_id_ >= kNumCowPictures) next_picture_id_ = 0;
    current_picture_id_[position] = next_picture_id_;
    last_picture_take_time_[position] = last_call_time_;
    has_flashed_[position] = false;
}

void Smartphones::NoMorePictures(void) {
    for (int i = 0; i < 4; i++) {
        show_pictures_[i] = false;
    }
}


int Smartphones::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    bool is_scene_finished = false;

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
    const char *kMinaTextures[kNumCowPictures] = {
        "Minamisoma_08.png",
        "Minamisoma_02.png",
        "Minamisoma_03.png",
        "Minamisoma_04.png",
        "Minamisoma_05.png",
        "Minamisoma_06.png",
        "Minamisoma_01.png",
        "Minamisoma_07.png",
    };
    const float kCowShotTimes[kNumCowPictures] = {
        //-10.0f,  // Shot was taken before scene starts
        16.0f,
        23.5f,
        32.5f,
        46.0f,
        54.0f,
        60.0f,
        72.0f,
        1000.0f, // whateffs
    };
    const float kMinaShowTime[kNumCowPictures] = {
        8.5f,
        27.0f,
        34.0f,
        38.5f,
        42.0f,
        69.0f,
        72.0f,
        74.5f
    };
    const int kMinaPicturePosition[kNumCowPictures] = {
        1,
        0,
        2,
        3,
        0,
        1,
        3,
        2
    };

    // Videos only during Cows
    // left, bottom, right
    const float kVideoSkip[3] = { 4.63f, 4.80f, 3.41f };
    const float kVideoDuration[2] = {80.0f, 85.0f};
    const char *kLeftVideo = "Kuhe_N4.wmv";
    const char *kBottomVideo = "Kuhe_R4.wmv";
    const char *kRightVideo = "Kuhe_Y3.wmv";
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (has_white_fade_) {
        to_white_ += (time - last_call_time_) * 1.0f;
        if (to_white_ > 1.75f) {
            to_white_ = 2.0f;
            is_scene_finished = true;
        }
    } else {
        to_white_ -= (time - last_call_time_) * 0.5f;
        if (to_white_ < 0.0f) to_white_ = 0.0f;
    }

    float video_time = time - video_start_time_;
    float last_video_time = last_call_time_ - video_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    if (scene_ == SM_KUHE) {
        for (int i = 0; i < kNumCowPictures; i++) {
            if (video_time >= kCowShotTimes[i] && last_video_time < kCowShotTimes[i]) {
                TakeNextPicture();
            }
        }
    } else {
        for (int i = 0; i < kNumCowPictures; i++) {
            if (video_time >= kMinaShowTime[i] && last_video_time < kMinaShowTime[i]) {
                TakeNextPicture(kMinaPicturePosition[i]);
            }
        }
    }

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID("smartphones_drawings_one.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Draw the cow picture
    // Maybe I show these after the videos and cut them perfectly (not needed)
    for (int picture_position = 0; picture_position < 4; picture_position++) {
        float picture_time = time - last_picture_take_time_[picture_position];
        float x_shift = 0.0f;
        float y_shift = 0.0f;
        const float kSeekTime = 0.8f;
        const float kFlashTime = 0.2f;
        if (picture_time < kSeekTime) {
            x_shift = (1.0f - picture_time / kSeekTime) * 0.025f * sinf(time * 2.3f) + 0.01f * cosf(time * 7.3f) - 0.005f * cosf(time * 23.5f);
            y_shift = (1.0f - picture_time / kSeekTime) * 0.025f * cosf(time * 3.1f) - 0.01f * sinf(time * 8.1f) + 0.005f * sinf(time * 27.4f);
        }
        const char *textureName = kCowTextures[current_picture_id_[picture_position]];
        if (scene_ != SM_KUHE) textureName = kMinaTextures[current_picture_id_[picture_position]];
        if (!show_pictures_[picture_position]) textureName = "black.png";
        if (textureManager.getTextureID(textureName, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float border_size = 0.04f;
        // Use texture coordinates for shaking
        switch (picture_position) {
        case 0:  // CENTER
        default:
            DrawQuad(-0.285f, 0.28f, 0.762f, -0.052f,
                border_size + x_shift, 1.0f - border_size + x_shift, border_size + y_shift, 1.0f - border_size + y_shift,
                1.0f);
            break;
        case 1:  // LEFT
            DrawQuad(-0.711f, -0.449f, 0.476f, -0.03f,
                border_size + x_shift, 1.0f - border_size + x_shift,
                border_size + y_shift, 1.0f - border_size + y_shift,
                1.0f);
            break;
        case 2:  // BOTTOM
            DrawQuad(-0.418f, -0.081f, 0.212f, -0.453f,
                border_size + x_shift, 1.0f - border_size + x_shift,
                border_size + y_shift, 1.0f - border_size + y_shift,
                1.0f);
            break;
        case 3:  // RIGHT
            DrawQuad(0.424f, 0.7075f, 0.73f, 0.136f,
                border_size + x_shift, 1.0f - border_size + x_shift,
                border_size + y_shift, 1.0f - border_size + y_shift,
                1.0f);
            break;
        }

        float white_alpha = 0.0f;
        if (picture_time >= kSeekTime) {
            white_alpha = 1.0f - (picture_time - kSeekTime) / kFlashTime;
            if (!has_flashed_[picture_position]) {
                //PlaySound("textures/flash.wav", NULL, SND_ASYNC);
                has_flashed_[picture_position] = true;
            }
        }
        if (white_alpha < 0.0f) white_alpha = 0.0f;
        if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        switch (picture_position) {
        case 0:  // CENTER
        default:
            DrawQuad(-0.285f, 0.28f, 0.762f, -0.052f, white_alpha);
            break;
        case 1:  // LEFT
            DrawQuad(-0.711f, -0.449f, 0.476f, -0.03f, white_alpha);
            break;
        case 2:  // BOTTOM
            DrawQuad(-0.418f, -0.0815f, 0.212f, -0.453f, white_alpha);
            break;
        case 3:  // RIGHT
            DrawQuad(0.424f, 0.7075f, 0.73f, 0.136f, white_alpha);
            break;
        }
    }

    // Draw the videos from Kuhe
    // Left
    if (scene_ == SM_KUHE) {
        if (textureManager.getVideoID(kLeftVideo, &tex_id, error_string,
            video_time + kVideoSkip[0]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.711f, -0.449f, 0.476f, -0.03f,
            0.175f, 0.825f, 0.0f, 1.0f,
            1.0f);

        // Bottom
        if (textureManager.getVideoID(kBottomVideo, &tex_id, error_string,
            video_time + kVideoSkip[1]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.418f, -0.0815f, 0.212f, -0.453f,
            0.2f, 0.8f, 0.0f, 1.0f,
            1.0f);

        // Right
        if (textureManager.getVideoID(kRightVideo, &tex_id, error_string,
            video_time + kVideoSkip[2]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(0.424f, 0.7075f, 0.73f, 0.136f,
            0.2f, 0.8f, 0.0f, 1.0f,
            1.0f);
    }

    // Draw the phones (exclude bottom):
    //if (textureManager.getTextureID("smartphones_room_nobottom.png", &tex_id, error_string)) {
    if (textureManager.getTextureID("smartphones_room.png", &tex_id, error_string)) {
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
        NO_SCENE_BRIGHTNESS, NO_SCENE_BRIGHTNESS, NO_SCENE_BRIGHTNESS, to_white_);

    // Panya in the watch
    switch (scene_) {
    case SM_KUHE:
        if (video_time >= 4.0f && last_video_time < 4.0f) NextPanya();
        if (video_time >= 9.0f && last_video_time < 9.0f) NextPanya();
        break;
    case SM_MINAMISOMA:
        if (video_time >= 32.5f && last_video_time < 32.5f) NextPanya();
        break;
    }
    if (current_panya_id_ >= 0) {
        const char *texture_name = "panya_ok.png";
        if (scene_ == SM_KUHE) texture_name = "panya_begeistert.png";
        if (textureManager.getTextureID(texture_name, &tex_id,
            error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        float panya_time = time - panya_start_time_;
        float size = panya_time * 4.0f - 1.5f;
        if (size < 0.0f) size = 0.0f;
        if (size > 1.0f) size = 1.0f;
        float cx = 0.5f * (-0.665f + -0.46f);
        float cy = 0.5f * (0.814f + 0.476f);
        float l = (1.0f - size) * cx + size * -0.665f;
        float r = (1.0f - size) * cx + size * -0.46f;
        float t = (1.0f - size) * cy + size * 0.814f;
        float b = (1.0f - size) * cy + size * 0.476f;
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float alpha = (4.0f - panya_time);
        if (alpha < 0) alpha = 0;
        if (alpha > 1) alpha = 1;
        DrawQuad(l, r, t, b, alpha);
    }

    // Darkening borders
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID("vignette.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    float video_duration = kVideoDuration[0];
    if (scene_ == SM_MINAMISOMA) video_duration = kVideoDuration[1];
    if (video_time > video_duration) {
        has_white_fade_ = true;
    }

    last_call_time_ = time;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (is_scene_finished) return 1;
    return 0;
}