#include "stdafx.h"
#include "Car.h"
#include "chockNgt.h"
#include "Configuration.h"
#include "Smartphones.h"
#include "Audio.h"

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
    show_gps_ = false;
    gps_start_time_ = last_call_time_ - 100.0f;  // Make sure that GPS isn't showing
}

int Car::Draw(float time) {
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;
    char error_string[MAX_ERROR_LENGTH+1];

    if (scene_ == KUHE) {
        last_call_time_ = time;
        return smartphones_.Draw(time);
    }

    // Time spent on GPS before it's size reduced
    const float kGPSDurations[] = {
        12.0f,  // Beginning draw Fukushima Daiishi
        6.0f,  // Kreuze no speech
    };
    const float kGPSFadeOutDuration[] = {
        1.0f,  // Beginning to avoid seek stutter
        6.0f,  // Polizei "Kreuze"
    };

    // left, center, right
    const float kVideoStartDelay[][3] = {
        {22.8f, 0.0f, 11.0f},
        {0.0f, 0.0f, 0.0f},  // No more delays happen after this
    };

    // left, center, right
    const float kVideoSkip[][3] = {
        {0.0f, 0.0f, 0.0f},  // Begruessung needs start delay...
        {0.0f, 0.0f, 0.0f},  // Tomobe has no sound... need to find proper delays
        {9.35f, 8.09f, 7.80f},  // SIEVERT
        {4.00f /* Random guess */, 11.25f, 9.86f},  // TAMURA
        {17.18f, 14.80f, 12.80f},  // 13Katsurao
        {5.20f, 6.4f, 5.11f},  // 14 Katsurao
        {0.0f, 3.0f /*3.27f / 2.27?*/, 3.65f},  // Abschied
        {10.25f, 8.38f, 9.04f},  // Zahnarzt
        {4.0f, 5.63f, 6.90f},  // Polizei
        {0.0f, 0.0f, 0.0f},  // Kühe
        {0.16f, 4.825f, 3.42f}
    };

    const float kVideoDuration[] = {
        43.0f,  //BEGRUSSUNG = 0
        80.0f,  // TOMOBE,
        137.0f,  // SIEVERT
        73.0f,  // TAMURA,
        181.5f,  // KATSURAO13
        175.0f,  // KATSURAO14
        6.0f,  // ABSCHIED,
        162.0f,  // ZAHNARZT,
        331.0f,  // POLIZEI,
        78.0f,  // KUHE,
        82.0f  // WOHIN
    };

    // The video in the center
    const char *kDriverVideo[] = {
        "Begrussung_R3.wmv",  //BEGRUSSUNG = 0
        "Tomobe_R2.wmv",  // TOMOBE,
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
        "Polizei_N3.wmv",
        NULL, // KUHE
        "Wohin_R5.wmv"
    };

    const char *kRightVideo[] = {
        "Begrussung_Y2.wmv",
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
                audio_.PlaySound("Wohin_N6Y6R5.wav", 0, false, -1, error_string);
                break;
            case KUHE:
                scene_ = KUHE;
                smartphones_.UpdateTime(time);
                smartphones_.StartScene(SM_KUHE);
                break;
            case ZAHNARZT:  // Probably not used due to GPS
                scene_ = ZAHNARZT;
                video_start_time_ = time;
                has_white_fade_ = false;
                audio_.PlaySound("Zahnarzt_N1Y1R2.wav", 0, false, -1, error_string);
                break;
            case POLIZEI:  // Probably not used due to GPS
                scene_ = POLIZEI;
                video_start_time_ = time;
                has_white_fade_ = false;
                audio_.PlaySound("Polizei_Y5R5.wav", 0, false, -1, error_string);
                break;
            case END_IT:
            default:
                is_scene_finished = true;
                break;
            }
        }
    } else {
        to_white_ -= (time - last_call_time_) * 0.5f;
        if (to_white_ < 0.0f) to_white_ = 0.0f;
    }

    float gps_size = 0.0f;
    float gps_time = time - gps_start_time_;
    if (show_gps_) {
        gps_size = gps_time * 0.5f;
        if (gps_size > 1.0f) gps_size = 1.0f;
        gps_size = 0.5f - cosf(gps_size * PIF) * 0.5f;  // Make it smooth
    } else {
        float delay = kGPSFadeOutDuration[0];
        switch(scene_) {
        case ZAHNARZT:
        default:
            delay = kGPSFadeOutDuration[0];
            break;
        case POLIZEI:
            delay = kGPSFadeOutDuration[1];
            break;
        }
        gps_size = gps_time * 0.5f - delay;
        if (gps_size < 0.0f) gps_size = 0.0f;
        if (gps_size > 1.0f) gps_size = 1.0f;
        gps_size = 0.5f + cosf(gps_size * PIF) * 0.5f;  // Make it smooth
    }

    float video_time = time - video_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Default background is white or gray
    float brightness = 1.0f;
    switch (scene_) {
    case BEGRUSSUNG:
    case TOMOBE:
    case SIEVERT:
    case ABSCHIED:
    case ZAHNARZT:
    case POLIZEI:
        brightness = 120.0f / 255.0f;
        break;
    case TAMURA:
    case KATSURAO13:
    case KATSURAO14:
    case KUHE:
    case WOHIN:
    case END_IT:
    default:
        brightness = 1.0f;
        break;
    }
    glClearColor(brightness, brightness, brightness, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
    } else {
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
    switch (scene_) {
    case BEGRUSSUNG:
    case TOMOBE:
    case SIEVERT:
    case ABSCHIED:
    case ZAHNARZT:
    case POLIZEI:
        texture_name = "car_room_night.png";
        break;
    case TAMURA:
    case KATSURAO13:
    case KATSURAO14:
    case KUHE:
    case WOHIN:
    case END_IT:
    default:
        texture_name = "car_room.png";
        break;
    }
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // GPS
    float l = gps_size * -0.9f + (1.0f - gps_size) * -0.3475f;
    float r = gps_size * 0.9f + (1.0f - gps_size) * -0.0325f;
    float t = gps_size * 0.95f + (1.0f - gps_size) * -0.052f;
    float b = gps_size * -0.85f + (1.0f - gps_size) * -0.364f;
    //if (textureManager.getTextureID("map_5_10_platt_small.png", &tex_id, error_string) < 0) {
    if (textureManager.getTextureID("map_5_10_platt_blur.png", &tex_id, error_string) < 0) {
    //if (textureManager.getTextureID("gps_schrift.png", &tex_id, error_string) < 0) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    float large_alpha = 8.0f * gps_size;
    if (large_alpha > 1.0f) large_alpha = 1.0f;
    DrawQuad(l, r, t, b, 1.0f);  // The small version of the GPS
    if (textureManager.getTextureID("map_5_10_platt.png", &tex_id, error_string) < 0) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(l, r, t, b, large_alpha);  // The large version of the GPS
    // Draw the flashing Fukushima for destination
    if (scene_ == BEGRUSSUNG) {
        float blinking = 0.0f;
        if (gps_time > 4.0f) blinking = 0.5f - cosf((gps_time - 4.0f) * 4.0f);
        if (textureManager.getTextureID("map_5_10_platt_nodaiishi.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(l, r, t, b, large_alpha * blinking);  // The large version of the GPS
    }
    // Draw Kreuze
    // Draw the flashing Fukushima for destination
    if (scene_ == SIEVERT || scene_ == POLIZEI) {
        float blinking = 0.0f;
        blinking = 0.5f - cosf((gps_time - 4.0f) * 4.0f);
        blinking = 0.6f + 0.4f * blinking;  // don't blink so much
        if (textureManager.getTextureID("map_5_10_platt_kreuze.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(l, r, t, b, large_alpha * blinking);  // The large version of the GPS
    }

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
            if (!show_gps_) {
                show_gps_ = true;
                gps_start_time_ = time;
            } else {
                if (gps_time > kGPSDurations[0]) {
                    show_gps_ = false;
                    gps_start_time_ = time;
                    scene_ = ZAHNARZT;
                    video_start_time_ = time;
                    has_white_fade_ = false;
                    audio_.PlaySound("Zahnarzt_N1Y1R2.wav", 0, false, -1, error_string);
                    break;
                }
            }
            break;
        case SIEVERT:  // Automatically moves on to Polizei
            //has_white_fade_ = true;
            //next_scene_ = POLIZEI;
            if (!show_gps_) {
                show_gps_ = true;
                gps_start_time_ = time;
            } else {
                if (gps_time > kGPSDurations[1]) {
                    show_gps_ = false;
                    gps_start_time_ = time;
                    scene_ = POLIZEI;
                    video_start_time_ = time;
                    has_white_fade_ = false;
                    audio_.PlaySound("Polizei_Y5R5.wav", 0, false, -1, error_string);
                    break;
                }
            }
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



