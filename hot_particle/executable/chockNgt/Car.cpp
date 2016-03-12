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
    next_scene_ = END_IT;
}

int Car::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;    

    if (scene_ == KUHE) {
        last_call_time_ = time;
        return smartphones_.Draw(time);
    }

    // left, center, right
    const float kVideoStartDelay[][3] = {
        {23.1f, 0.0f, 10.35f},
        {0.0f, 0.0f, 0.0f},  // No more delays happen after this
    };

    // left, center, right
    const float kVideoSkip[][3] = {
        {0.0f, 0.0f, 1.0f},  // Begruessung needs start delay...
        {0.0f, 0.0f, 0.0f},  // Tomobe has no sound... need to find proper delays
        {9.35f, 8.09f, 7.80f},  // SIEVERT
        {4.00f /* Random guess */, 11.25f, 9.86f},  // TAMURA
        {17.18f, 14.80f, 12.80f},  // 13Katsurao
        {5.20f, 6.4f, 5.11f},  // 14 Katsurao
        {0.0f, 3.27f /*2.27?*/, 3.65f},  // Abschied
        {10.25f, 8.38f, 9.04f},  // Zahnarzt
        {0.0f /* FEHLT */, 5.63f, 6.90f},  // Polizei
        {0.0f, 0.0f, 0.0f},  // Kühe
        {0.16f, 4.825f, 3.42f}
    };

    const float kVideoDuration[] = {
        43.0f,  //BEGRUSSUNG = 0
        80.0f,  // TOMOBE,
        137.0f,  // SIEVERT
        73.0f,  // TAMURA,
        199.0f,  // KATSURAO13
        159.0f,  // KATSURAO14
        6.0f,  // ABSCHIED,
        162.0f,  // ZAHNARZT,
        331.0f,  // POLIZEI,
        78.0f,  // KUHE,
        82.0f  // WOHIN
    };

    // The video in the center
    const char *kDriverVideo[] = {
        "Begrussung_R3.wmv",  //BEGRUSSUNG = 0
        "Tomobe_R1.wmv",  // TOMOBE,
        "Sievert_R1.wmv",  // SIEVERT
        "Tamura_N3.wmv",  // TAMURA,
        "13Katsurao_N1.wmv",  // KATSURAO13
        "14Katsurao_N1.wmv",  // KATSURAO14
        "Abschied_R2.wmv",  // ABSCHIED,
        "Zahnarzt_R2.wmv",  // ZAHNARZT,
        "Polizei_R5.wmv",  // POLIZEI,
        NULL,  // KUHE,
        "Wohin_N6.wmv"  // WOHIN
    };

    const char *kLeftVideo[] = {
        "Begrussung_N1.wmv",
        "Tomobe_N2.wmv",
        "Sievert_N1.wmv",
        "Tamura_R1.wmv",
        "13Katsurao_R3.wmv",
        "14Katsurao_R1.wmv",
        "Abschied_N1.wmv",
        "Zahnarzt_N1.wmv",
        NULL, // not done yet
        NULL, // KUHE
        "Wohin_R5.wmv"
    };

    const char *kRightVideo[] = {
        "Begrussung_Y1.wmv",
        "Tomobe_Y2.wmv",
        "Sievert_Y1.wmv",
        "Tamura_Y1.wmv",
        "13Katsurao_Y3.wmv",
        "14Katsurao_Y4.wmv",
        "Abschied_Y1.wmv",
        "Zahnarzt_Y1.wmv",
        "Polizei_Y4.wmv",  // Y4, not 5?
        NULL,  // KUHE
        "Wohin_Y6.wmv"
    };

    if (has_white_fade_) {
        to_white_ += (time - last_call_time_) * 1.0f;
        if (to_white_ > 1.75f) {
            to_white_ = 2.0f;
            switch (next_scene_) {
            case WOHIN:
                scene_ = WOHIN;
                video_start_time_ = time;
                has_white_fade_ = false;
                PlaySound("textures/Wohin_N6Y6R5.wav", NULL, SND_ASYNC);
            case KUHE:
                scene_ = KUHE;
                smartphones_.UpdateTime(time);
                smartphones_.StartScene(SM_KUHE);
                break;
            case ZAHNARZT:
                scene_ = ZAHNARZT;
                video_start_time_ = time;
                has_white_fade_ = false;
                PlaySound("textures/Zahnarzt_N1Y1R2.wav", NULL, SND_ASYNC);
                break;
            case POLIZEI:
                scene_ = POLIZEI;
                video_start_time_ = time;
                has_white_fade_ = false;
                PlaySound("textures/Polizei_Y5R5.wav", NULL, SND_ASYNC);
                break;
            case END_IT:
            default:
                is_scene_finished = true;
                break;
            }
        }
    } else {
        to_white_ -= time - last_call_time_;
        if (to_white_ < 0.0f) to_white_ = 0.0f;
    }

    float video_time = time - video_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Default background is white
    if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Driver Video (always do it?)
    if (kDriverVideo[scene_] && video_time > kVideoStartDelay[scene_][1] &&
        video_time <= kVideoDuration[scene_] + 2.0f) {
        texture_name = kDriverVideo[scene_];
        if (textureManager.getVideoID(texture_name, &tex_id, error_string,
            video_time - kVideoStartDelay[scene_][1] + kVideoSkip[scene_][1]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    else {
        if (textureManager.getTextureID("black.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-0.3225f, 0.3225f, 0.72f, -0.054f, 1.0f);

    // Left Video (always do it?)
    if (kLeftVideo[scene_] && video_time > kVideoStartDelay[scene_][0] &&
        video_time <= kVideoDuration[scene_] + 2.0f) {
        texture_name = kLeftVideo[scene_];
        if (textureManager.getVideoID(texture_name, &tex_id, error_string,
            video_time - kVideoStartDelay[scene_][0] + kVideoSkip[scene_][0]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    else {
        if (textureManager.getTextureID("black.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    // Hier muesste ich zwischen 3/4 und 9/16 unterscheiden
    // 9/16
    DrawQuad(-0.7625f, -0.39875f, 0.72f, -0.054f,
        0.289f, 0.711f, 0.1f, 0.9f,
        1.0f);

    // Right Video (always do it?)
    if (kRightVideo[scene_] && video_time > kVideoStartDelay[scene_][2] &&
        video_time <= kVideoDuration[scene_] + 2.0f) {
        texture_name = kRightVideo[scene_];
        if (textureManager.getVideoID(texture_name, &tex_id, error_string,
            video_time - kVideoStartDelay[scene_][2] + kVideoSkip[scene_][2]) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    else {
        if (textureManager.getTextureID("black.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    // Hier muesste ich zwischen 3/4 und 9/16 unterscheiden
    // 3/4
    DrawQuad(0.39875f, 0.7625f, 0.72f, -0.054f,
        0.21875f, 0.78125f, 0.0f, 1.0f,
        1.0f);

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

    // Check whether a video ended (state machine code here...)
    if (video_time > kVideoDuration[scene_]) {
        switch (scene_) {
        case BEGRUSSUNG:  // Automatically moves on to Zahnarzt
            has_white_fade_ = true;
            next_scene_ = ZAHNARZT;
            break;
        case SIEVERT:  // Automatically moves on to Polizei
            has_white_fade_ = true;
            next_scene_ = POLIZEI;
            break;
        case KATSURAO13: // Automatically moves on to Kuhe
            has_white_fade_ = true;
            next_scene_ = KUHE;
            break;
        case KATSURAO14: // Automatically moves on to Wohin
            has_white_fade_ = true;
            next_scene_ = WOHIN;
            break;
        case TAMURA:
        case TOMOBE:
        case ABSCHIED:
        case ZAHNARZT:
        case POLIZEI:
        case KUHE:
        case WOHIN:
        default:
            next_scene_ = END_IT;
            has_white_fade_ = true;
            break;
        }
    }

    if (is_scene_finished) return 1;  // No fade-out so far.
    return 0;  // no error
}



