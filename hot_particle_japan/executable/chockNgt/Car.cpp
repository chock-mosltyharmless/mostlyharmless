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
    current_panya_id_ = -1;
    panya_start_time_ = last_call_time_;
}

int Car::Draw(float time) {
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;
    char error_string[MAX_ERROR_LENGTH + 1];

    if (scene_ == KUHE) {
        last_call_time_ = time;
        return smartphones_.Draw(time);
    }

    // Panya whole script
    const int kMaxNumPanyas = 12;
    const char *kPanyaNames[11][kMaxNumPanyas] = {
        /*BEGRUSSUNG*/ {"panya_begrussung.png", "panya_ok.png"},
        /*TOMOBE    */ {"panya_ok.png", "panya_nachdenklich.png"},
        /*SIEVERT   */ {"panya_wow.png", "panya_sauer.png", "panya_fragend.png"},
        /*TAMURA    */ {"panya_fragend.png", "panya_ok.png", "panya_ok.png", "panya_ok.png"},
        /*KATSU 13  */ {"panya_fragend.png"},
        /*KATSU 14  */ {"panya_nachdenklich.png", "panya_sauer.png", "panya_wow.png"},
        /*ABSCHIED  */ {"panya_begrussung.png"},
        /*ZAHNARZT  */ {"panya_sauer.png"},
        /*POLIZEI   */ {"panya_nachdenklich.png", "panya_fragend.png", "panya_wow.png", "panya_fragend.png", "panya_wow.png", "panya_wow.png", "panya_begeistert.png", "panya_nachdenklich.png", "panya_fragend.png", "panya_wow.png", "panya_wow.png", "panya_wow.png"},
        /*KUHE      */ {},
        /*WOHIN     */ {"panya_wow.png", "panya_nachdenklich.png", "panya_wow.png", "panya_sauer.png", "panya_fragend.png"}
    };
    const float kPanyaStartTimes[11][kMaxNumPanyas] = {
        /*BEGRUSSUNG*/{ 3.0f, 39.0f },//{3.0f, 41.0f},
        /* TOMOBE   */{ 30.0f, 52.0f },//{38.0f, 60.0f},
        /* SIEVERT  */ {2.0f, 14.0f, 141.0f},
        /* TAMURA   */{ 8.0f, 42.0f, 48.0f, 156.0f },//{8.0f, 47.0f, 53.0f, 56.0f},
        /* KATSU 13 */ {173.5f},
        /* KATSU 14 */ {7.0f, 62.0f, 157.0f},
        /* ABSCHIED */ {6.5f},
        /* ZAHARZT  */ {155.0f},
        /* POLIZEI  */{ 72.0f, 79.0f, 87.0f, 91.0f, 116.0f, 155.0f, 161.0f, 224.0f, 233.0f, 255.0f, 274.0f, 306.0f },//{72.0f, 83.0f, 87.0f, 93.0f, 116.0f, 155.0f, 165.0f, 227.0f, 239.0f, 255.0f, 274.0f, 310.0f},
        /* KUHE     */ {},
        /* WOHIN    */ {2.0f, 10.0f, 30.0f, 46.0f, 76.0f}
    };

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
        {23.2f, 0.0f, 11.0f},
        {0.0f, 0.0f, 0.0f},  // No more delays happen after this
    };

    // left, center, right
    const float kVideoSkip[][3] = {
#if 0
        {0.0f, 0.0f, 0.0f}, //
#else 
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
#endif
    };

    const float kVideoDuration[] = {
        43.0f,  //BEGRUSSUNG = 0
        80.0f,  // TOMOBE,
        137.0f,  // SIEVERT
        73.0f,  // TAMURA,
        181.5f,  // KATSURAO13
        168.0f,  // KATSURAO14
        9.0f,  // ABSCHIED,
        162.0f,  // ZAHNARZT,
        328.0f,  // POLIZEI,
        78.0f,  // KUHE,
        84.0f  // WOHIN
    };

    const float kDriverMoveRight[] = {
        0.2f,  //BEGRUSSUNG = 0
        0.2f,  // TOMOBE,
        0.3f,  // SIEVERT
        0.2f,  // TAMURA,
        0.1f,  // KATSURAO13
        0.0f,  // KATSURAO14
        0.3f,  // ABSCHIED,
        0.3f,  // ZAHNARZT,
        0.3f,  // POLIZEI,
        0.0f,  // KUHE,
        0.0f  // WOHIN
    };
    const float kDriverLeftFade[] = {
        0.3f,  //BEGRUSSUNG = 0
        0.3f,  // TOMOBE,
        0.4f,  // SIEVERT
        0.3f,  // TAMURA,
        0.2f,  // KATSURAO13
        0.1f,  // KATSURAO14
        0.4f,  // ABSCHIED,
        0.4f,  // ZAHNARZT,
        0.4f,  // POLIZEI,
        0.3f,  // KUHE,
        0.2f  // WOHIN
    };

    const char *kSubitle[] = {
        "Begrussung_R3.txt",  //BEGRUSSUNG = 0
        "Tomobe_R2.txt",  // TOMOBE,
        "Sievert_R1.txt",  // SIEVERT
        "Tamura_N3.txt",  // TAMURA,
        "13Katsurao_N1.txt",  // KATSURAO13
        "14Katsurao_N1.txt",  // KATSURAO14
        "Abschied_R2.txt",  // ABSCHIED,
        "Zahnarzt_R2.txt",  // ZAHNARZT,
        "Polizei_R5.txt",  // POLIZEI,
        NULL,  // KUHE,
        "Wohin_N6.txt"  // WOHIN
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
                subtitle_start_time_ = time;
                subtitle_script_ = "Wohin_N6Y6R5.txt";
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
                subtitle_start_time_ = time;
                subtitle_script_ = "Zahnarzt_N1Y1R2.txt";
                break;
            case POLIZEI:  // Probably not used due to GPS
                scene_ = POLIZEI;
                video_start_time_ = time;
                has_white_fade_ = false;
                audio_.PlaySound("Polizei_Y5R5.wav", 0, false, -1, error_string);
                subtitle_start_time_ = time;
                subtitle_script_ = "Polizei_Y5R5.txt";
                break;
            case END_IT:
            default:
                is_scene_finished = true;
                break;
            }
        }
    }
    else {
        to_white_ -= (time - last_call_time_) * 0.5f;
        if (to_white_ < 0.0f) to_white_ = 0.0f;
    }

    float video_time = time - video_start_time_;
    float last_video_time = last_call_time_ - video_start_time_;
    if (video_time < 0.0f) video_time = 0.0f;

    // 10% schneller on wohin
    if (scene_ == WOHIN) {
        video_time *= 1.11104f;
    }

    float gps_l, gps_r, gps_t, gps_b;
    float gps_size = 0.0f;
    float gps_time = time - gps_start_time_;
    if (show_gps_) {
        gps_size = gps_time * 0.5f;
        if (gps_size > 1.0f) gps_size = 1.0f;
        gps_size = 0.5f - cosf(gps_size * PIF) * 0.5f;  // Make it smooth
    }
    else {
        float delay = kGPSFadeOutDuration[0];
        switch (scene_) {
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
    gps_r = gps_size * 0.8f + (1.0f - gps_size) * 0.686f;
    gps_l = gps_size * -0.8f + (1.0f - gps_size) * 0.475f;
    gps_t = gps_size * 1.2f + (1.0f - gps_size) * -0.142f;
    gps_b = gps_size * -1.2f + (1.0f - gps_size) * -0.518f;
    // Special case: Tomobe always has a smaller map
    if (scene_ == TOMOBE) {
        gps_size = (video_time - 4.0f) * 0.5f;
        if (gps_size < 0.0f) gps_size = 0.0f;
        if (gps_size > 1.0f) gps_size = 1.0f;
        gps_r = gps_size * 0.8f*0.67f + (1.0f - gps_size) * 0.686f;
        gps_l = gps_size * -0.8f*0.67f + (1.0f - gps_size) * 0.475f;
        gps_t = gps_size * 1.2f*0.67f + (1.0f - gps_size) * -0.142f;
        gps_b = gps_size * -1.2f*0.67f + (1.0f - gps_size) * -0.518f;
    }

    float driving_speed = 0.0f;
    switch (scene_) {
    case ZAHNARZT:
        driving_speed = 1.0f;
        break;
    case SIEVERT:
        driving_speed = 1.0f;
        break;
    case POLIZEI:
        driving_speed = 1.0f;
        if (video_time > 110.0f) driving_speed = 1.0f - (video_time - 110.0f) / 2.0f;
        if (video_time >= 110.0f && last_video_time < 110.0f) audio_.StopSound(2, 24.0f, error_string);
        if (video_time > 156.0f) driving_speed = (video_time - 156.0f) / 2.0f;
        if (video_time >= 156.0f && last_video_time < 156.0f) audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
        if (video_time > 298.0f) driving_speed = 1.0f - (video_time - 298.0f) / 4.0f;
        if (video_time >= 298.0f && last_video_time < 298.0f) audio_.StopSound(2, 8.0f, error_string);
        break;
    case KATSURAO13:
        driving_speed = 1.0f;
        if (video_time > 92.0f) driving_speed = 1.0f - (video_time - 92.0f) / 8.0f;
        if (video_time >= 92.0f && last_video_time < 92.0f) audio_.StopSound(2, 4.0f, error_string);
        break;
    case KATSURAO14:
        driving_speed = 1.0f;
        break;
    case WOHIN:
        driving_speed = 1.0f;
        break;
    default:
        driving_speed = 0.0f;
        break;
    }
    if (driving_speed < 0.0f) driving_speed = 0.0f;
    if (driving_speed > 1.0f) driving_speed = 1.0f;

    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha

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
    float n1 = 0.02f * driving_speed * (sinf(video_time * 9.0f) + 0.4f * cosf(video_time * 21.0f) + 0.2f * cosf(video_time * 41.0f));
    float n2 = 0.02f * driving_speed * (cosf(video_time * 7.7f) + 0.4f * cosf(video_time * 18.5f) + 0.2f * sinf(video_time * 53.4f));
    float n3 = 0.02f * driving_speed * (cosf(video_time * 5.3f) + 0.4f * sinf(video_time * 28.5f) + 0.2f * cosf(video_time * 38.4f));
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
    float cut_x = 0.5f * (kDriverMoveRight[scene_] * 0.3225f + (1.0f - kDriverMoveRight[scene_]) * -0.3225f);
    float fade_x_l = 0.5f * ((kDriverLeftFade[scene_] - 0.1f) * 0.3225f + (1.0f - kDriverLeftFade[scene_] + 0.1f) * -0.3225f);
    float fade_x_r = 0.5f * (kDriverLeftFade[scene_] * 0.3225f + (1.0f - kDriverLeftFade[scene_]) * -0.3225f);
    DrawQuad(cut_x, 0.339f, 0.72f, -0.054f,
             0.0f + n1, -kDriverMoveRight[scene_] + 0.9f + n1, 0.0f + n2, 1.0f + n2,
             1.0f);
#if 0
    DrawQuadColor(-0.3225f, fade_x_l, 0.72f, -0.054f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    DrawBlackFade(fade_x_l, fade_x_r, 0.72f, -0.054f);
#endif

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
       0.289f + 0.6f * n2, 0.711f + 0.6f*n2, 0.1f + 0.6f*n3, 0.9f + 0.6f *n3,
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
        0.21875f + 0.6f * n3, 0.78125f +  0.6f * n3, 0.0f + 0.6f * n1, 1.0f + 0.6f * n1,
        1.0f);

    // Room
    bool night_scene = false;
    switch (scene_) {
    case BEGRUSSUNG:
    case TOMOBE:
    case SIEVERT:
    case ABSCHIED:
    case ZAHNARZT:
    case POLIZEI:
        texture_name = "car_room_night.png";
        night_scene = true;
        break;
    case TAMURA:
    case KATSURAO13:
    case KATSURAO14:
    case KUHE:
    case WOHIN:
    case END_IT:
    default:
        texture_name = "car_room.png";
        night_scene = false;
        break;
    }
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Panya backdrop
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha
    if (textureManager.getTextureID("panya_car_blank.png", &tex_id, error_string) < 0) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-0.686f, -0.475f, -0.142f, -0.5118f, 1.0f);
    if (night_scene) {
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // glow
        glBlendFunc(GL_ONE, GL_ONE);  // Use pre-multiplied alpha
        texture_name = "kneipe_clock_glow.png";
        if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.686f - 0.45f, -0.475f + 0.45f, -0.142f + 0.6f, -0.5118f - 0.6f, 0.4f);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha
    }

    // GPS
    //if (textureManager.getTextureID("map_5_10_platt_small.png", &tex_id, error_string) < 0) {
    if (textureManager.getTextureID("map_5_10_small.png", &tex_id, error_string) < 0) {
    //if (textureManager.getTextureID("gps_schrift.png", &tex_id, error_string) < 0) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    float large_alpha = 4.0f * gps_size;
    if (large_alpha > 1.0f) large_alpha = 1.0f;
    DrawQuad(gps_l, gps_r, gps_t, gps_b, 1.0f);  // The small version of the GPS
    texture_name = "map_5_10.png";
    if (scene_ == TOMOBE) texture_name = "map_5_11.png";
    if (textureManager.getTextureID(texture_name, &tex_id, error_string) < 0) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(gps_l, gps_r, gps_t, gps_b, large_alpha);  // The large version of the GPS
    // Draw the flashing Fukushima for destination
    if (scene_ == BEGRUSSUNG) {
        float blinking = 0.0f;
        if (gps_time > 4.0f) blinking = 0.5f - cosf((gps_time - 4.0f) * 4.0f);
        if (textureManager.getTextureID("map_5_10_nodaiichi.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(gps_l, gps_r, gps_t, gps_b, large_alpha * blinking);  // The large version of the GPS
    }
    // Draw Kreuze
    // Draw the flashing Fukushima for destination
    if (scene_ == SIEVERT || scene_ == POLIZEI) {
        float blinking = 0.0f;
        blinking = 0.5f - cosf((gps_time - 4.0f) * 4.0f);
        blinking = 0.6f + 0.4f * blinking;  // don't blink so much
        if (textureManager.getTextureID("map_5_10_kreuze.png", &tex_id, error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(gps_l, gps_r, gps_t, gps_b, large_alpha * blinking);  // The large version of the GPS
    }
    // Glow for small
    if (night_scene) {
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBlendFunc(GL_ONE, GL_ONE);  // Use pre-multiplied alpha
        texture_name = "kneipe_clock_glow.png";
        if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(0.475f - 0.45f, 0.686f + 0.45f, -0.142f + 0.6f, -0.5118f - 0.6f, 0.4f * (1.0f - large_alpha));
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha
    }

    // Drawing Panya
    if (current_panya_id_ >= 0 && current_panya_id_ < kMaxNumPanyas &&
        kPanyaNames[scene_][current_panya_id_] != NULL) {
        const char *texture_name = kPanyaNames[scene_][current_panya_id_];
        if (textureManager.getTextureID(texture_name, &tex_id,
            error_string) < 0) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        float panya_time = time - panya_start_time_;
        float size = panya_time * 4.0f - 1.5f;
        if (size < 0.0f) size = 0.0f;
        if (size > 1.0f) size = 1.0f;
        float cx = 0.5f * (-0.475f + -0.686f);
        float cy = 0.5f * (-0.142f + -0.518f);
        float r = (1.0f - size) * cx + size * -0.475f;
        float l = (1.0f - size) * cx + size * -0.686f;
        float t = (1.0f - size) * cy + size * -0.142f;
        float b = (1.0f - size) * cy + size * -0.518f;
        glBindTexture(GL_TEXTURE_2D, tex_id);
        float alpha = (6.0f - panya_time);
        if (alpha < 0) alpha = 0;
        if (alpha > 1) alpha = 1;
        DrawQuad(l, r, t, b, alpha);
    }
    // And her scripting
    if (current_panya_id_ < kMaxNumPanyas - 1 && kPanyaNames[scene_][current_panya_id_ + 1] != NULL &&
        video_time >= kPanyaStartTimes[scene_][current_panya_id_ + 1] &&
        last_video_time < kPanyaStartTimes[scene_][current_panya_id_ + 1]) {
        NextPanya();
    }

    // Fade to white
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha
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

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Use pre-multiplied alpha

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
                    current_panya_id_ = -1;
                    audio_.PlaySound("Zahnarzt_N1Y1R2.wav", 0, false, -1, error_string);
                    audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
                    subtitle_start_time_ = time;
                    subtitle_script_ = "Zahnarzt_N1Y1R2.txt";
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
                    current_panya_id_ = -1;
                    audio_.PlaySound("Polizei_Y5R5.wav", 0, false, -1, error_string);
                    subtitle_start_time_ = time;
                    subtitle_script_ = "Polizei_Y5R5.txt";
                    // Audio was already there, no fade-in necesary.
                    //audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
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
            current_panya_id_ = -1;
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
            audio_.StopSound(2, 24.0f, error_string);
            break;
        }
    }

    if (is_scene_finished) return 1;  // No fade-out so far.
    return 0;  // no error
}



