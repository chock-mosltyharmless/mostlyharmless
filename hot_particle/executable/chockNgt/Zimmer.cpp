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
    has_white_fade_ = false;
    brightness_ = 0.0f;
    to_white_ = 0.0f;
    scene_ = MAERZ_11;
}

void Zimmer::StartScene(ZIMMER_SCENE scene) {
    PlaySound("textures/silence.wav", NULL, SND_ASYNC);
    scene_ = scene;
    EndKenchiro();
    switch(scene_) {
    case MAERZ_11:
        has_light_ = true;
        has_white_fade_ = false;
        brightness_ = 0.0f;
        to_white_ = 0.0f;
        break;
    default:
        has_light_ = true;
        has_white_fade_ = false;
        brightness_ = 1.0f;
        to_white_ = 1.0f;
        break;
    }
}

void Zimmer::EndScene(void) {
    PlaySound("textures/silence.wav", NULL, SND_ASYNC);
    has_light_ = true;
    has_white_fade_ = true;
    brightness_ = 1.0f;
    to_white_ = 0.0f;
}

int Zimmer::Draw(float time) {
    char error_string[MAX_ERROR_LENGTH + 1];
    GLuint tex_id;
    const char *texture_name;
    bool is_scene_finished = false;  // Set to true if it completely faded away

    float kFrameSkipTimes[8] = {6.0f, 7.0f, 5.5f, 14.0f, 7.0f, 6.5f,  // kenchiro
                                0.0f, 0.0f,  // Nakaba
    };
    float kFrameOpenTimes[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // kenchiro
                                0.0f, 0.0f,  // Nakaba
    };
    float kFrameCloseTimes[8] = {152.5f, 100.5f, 305.0f, 85.0f, 73.0f, 313.0f,  // kenchiro
                                 58.0f, 97.0f  // Nakaba
    };
    // 0: clock
    // 1: center
    int kVideoPosition[8] = {0, 0, 0, 0, 0, 0,  // Kenshiro
                             1, 1,  // Nakaba
    };
    const char *kKenchiroVideos[8] = {
        "S1_wachauf02_hell.wmv",
        "S20_Kitchomu_hell.wmv",
        "S23_Picasso03_hell.wmv",
        "S27_schwimmen03_hell.wmv",
        "S28_wasma02_hell.wmv",
        "S29_Kobe02_hell.wmv",
        "Naka_Erdbeben_01.wmv",
        "Naka_Zuhause_02.wmv"
    };
    const char *kRoomTextures[] = {
        "zuhause_room_maerz_11.png",  // MAERZ_11 = 0,
        "zuhause_room_april_09.png",  // ARPIL_09,
        "zuhause_room_april_16.png",  // APRIL_16,
        "zuhause_room_april_16.png",  // APRIL_17,
        "zuhause_room_april_21.png",  // APRIL_21,
        "zuhause_room_mai_10.png",  // MAI_10,
        "zuhause_room_juni_01.png",  // JUNI_01,
        "zuhause_room_juni_04.png",  // JUNI_04,
        "zuhause_room_juni_05.png",  // JUNI_05,
        "zuhause_room_juni_12.png",  // JUNI_12,
        "zuhause_room_juli_29.png",  // JULI_29,
        "zuhause_room_august_15.png",  // AUGUST_15,
        "zuhause_room_maerz_11.png",  // MARZ_11_END,
        "zuhause_room_september_14.png",  // UNKNOWN
        "proberaum_room.png",  // PROBERAUM
    };
    const char *kRoomNoClockTextures[] = {
        "zuhause_room_maerz_11_noclock.png",  // MAERZ_11 = 0,
        "zuhause_room_maerz_11_noclock.png",  // ARPIL_09,
        "zuhause_room_maerz_11_noclock.png",  // APRIL_16,
        "zuhause_room_maerz_11_noclock.png",  // APRIL_17,
        "zuhause_room_maerz_11_noclock.png",  // APRIL_21,
        "zuhause_room_mai_10_noclock.png",  // MAI_10,
        "zuhause_room_mai_10_noclock.png",  // JUNI_01,
        "zuhause_room_mai_10_noclock.png",  // JUNI_04,
        "zuhause_room_mai_10_noclock.png",  // JUNI_05,
        "zuhause_room_mai_10_noclock.png",  // JUNI_12,
        "zuhause_room_juli_29_noclock.png",  // JULI_29,
        "zuhause_room_august_15_noclock.png",  // AUGUST_15,
        "zuhause_room_maerz_11_noclock.png",  // MARZ_11_END,
        "zuhause_room_september_14_noclock.png",  // UNKNOWN
        "proberaum_room_noclock.png",  // PROBERAUM
    };
    const char *kRoomDrawingsTextures[] = {
        "zuhause_drawings_maerz_night.png",  // MAERZ_11 = 0,
        "zuhause_drawings_april_night.png",  // ARPIL_09,
        "zuhause_drawings_april.png",  // APRIL_16,
        "zuhause_drawings_april_night.png",  // APRIL_17,
        "zuhause_drawings_april.png",  // APRIL_21,
        "zuhause_drawings_mai_night.png",  // MAI_10,
        "zuhause_drawings_juni.png",  // JUNI_01,
        "zuhause_drawings_juni.png",  // JUNI_04,
        "zuhause_drawings_juni.png",  // JUNI_05,
        "zuhause_drawings_juni_night.png",  // JUNI_12,
        "zuhause_drawings_juli_night.png",  // JULI_29,
        "zuhause_drawings_august_night.png",  // AUGUST_15,
        "zuhause_drawings_maerz_night.png",  // MARZ_11_END,
        "zuhause_drawings_september_night.png",  // UNKNOWN
        "zuhause_drawings_mai.png",  // PROBERAUM
    };
    const char *kRoomLightings[] = {
        "zuhause_lighting_night.png",  // MAERZ_11 = 0,
        "zuhause_lighting_night.png",  // ARPIL_09,
        "zuhause_lighting_day.png",  // APRIL_16,
        "zuhause_lighting_night.png",  // APRIL_17,
        "zuhause_lighting_day.png",  // APRIL_21,
        "zuhause_lighting_night.png",  // MAI_10,
        "zuhause_lighting_day.png",  // JUNI_01,
        "zuhause_lighting_day.png",  // JUNI_04,
        "zuhause_lighting_day.png",  // JUNI_05,
        "zuhause_lighting_day.png",  // JUNI_12,
        "zuhause_lighting_night.png",  // JULI_29,
        "zuhause_lighting_night.png",  // AUGUST_15,
        "zuhause_lighting_night.png",  // MARZ_11_END,
        "zuhause_lighting_night.png",  // UNKNOWN
        "proberaum_lighting.png",  // PROBERAUM
    };
    const char *kRoomClocks[] = {
        "zuhause_clock_maerz_11.png",  // MAERZ_11 = 0,
        "zuhause_clock_maerz_11.png",  // ARPIL_09,
        "zuhause_clock_maerz_11.png",  // APRIL_16,
        "zuhause_clock_maerz_11.png",  // APRIL_17,
        "zuhause_clock_maerz_11.png",  // APRIL_21,
        "zuhause_clock_mai_10.png",  // MAI_10,
        "zuhause_clock_mai_10.png",  // JUNI_01,
        "zuhause_clock_mai_10.png",  // JUNI_04,
        "zuhause_clock_mai_10.png",  // JUNI_05,
        "zuhause_clock_mai_10.png",  // JUNI_12,
        "zuhause_clock_juli_29.png",  // JULI_29,
        "zuhause_clock_august_15.png",  // AUGUST_15,
        "zuhause_clock_maerz_11.png",  // MARZ_11_END,
        "zuhause_clock_september_14.png",  // UNKNOWN
        "zuhause_clock_maerz_11.png",  // PROBERAUM
    };

    // Adjust brightness according to time difference
    if (has_light_) {
        brightness_ += (time - last_call_time_) * 2.0f;
        if (brightness_ > 1.0f) brightness_ = 1.0f;
    } else {
        brightness_ -= time - last_call_time_;
        if (brightness_ < 0.0f) brightness_ = 0.0f;
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

    last_call_time_ = time;

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Adjust for overlapping nightmare...
    if (textureManager.getTextureID(kRoomDrawingsTextures[scene_], &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Kenshiro behind the clock
    if (draw_kenchiro_ && kVideoPosition[kenchiro_id_] == 0) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id, error_string,
                                      video_time + 0.5f + kFrameSkipTimes[kenchiro_id_]) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.665f, -0.46f, 0.814f, 0.476f, 0.3f, 0.65f, 0.25f, 0.8f, 1.0f);
    }

    // Video in main frame
    if (draw_kenchiro_ && kVideoPosition[kenchiro_id_] == 1) {
        if (textureManager.getVideoID(kKenchiroVideos[kenchiro_id_], &tex_id,
            error_string, video_time + 0.5f + kFrameSkipTimes[kenchiro_id_]) < 0) {
            MessageBox(mainWnd, error_string, "Texture manager get video ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-0.3f, 0.339f, 0.7f, -0.1f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
        if (video_time > kFrameCloseTime) draw_kenchiro_ = false;
        
        // Block TV
        if (textureManager.getTextureID("zuhause_block_tv.png", &tex_id, error_string)) {
            MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
            return -1;
        }
        glBindTexture(GL_TEXTURE_2D, tex_id);
        DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);
    }

    // Room
    texture_name = kRoomTextures[scene_];
    if (start_kenchiro && kVideoPosition[kenchiro_id_] == 0) texture_name = kRoomNoClockTextures[scene_];
    if (textureManager.getTextureID(texture_name, &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(-1.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    // Night lighting of the room
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    if (textureManager.getTextureID(kRoomLightings[scene_], &tex_id, error_string)) {
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

    // Draw opened lid
    if (start_kenchiro && kVideoPosition[kenchiro_id_] == 0) {
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
        texture_name = kRoomClocks[scene_];
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
    if (textureManager.getTextureID("white.png", &tex_id, error_string)) {
        MessageBox(mainWnd, error_string, "Could not get texture ID", MB_OK);
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DrawQuadColor(-1.0f, 1.0f, 1.0f, -1.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f - brightness_);
    
    if (is_scene_finished) return 1;
    return 0;  // no error
}

void Zimmer::StartKenchiro(void) {
    draw_kenchiro_ = true;
    kenchiro_start_time_ = last_call_time_;
    switch (scene_) {
    case MAERZ_11:
    case APRIL_09:
    case APRIL_16:
        kenchiro_id_ = 0;  // APRIL_21,
        PlaySound("textures/S1_wachauf02_nr_nomisa_skip6.wav", NULL, SND_ASYNC);
        break;
    case APRIL_17:
        kenchiro_id_ = 6;  // Naka Erdbeben
        PlaySound("textures/Naka_Erdbeben_01.wav", NULL, SND_ASYNC);
        break;
    case APRIL_21:
        kenchiro_id_ = 7;  // Naka Zuhause
        PlaySound("textures/Naka_Zuhause_02.wav", NULL, SND_ASYNC);
        break;
    case MAI_10:
    case JUNI_01:
    case JUNI_04:
    case JUNI_05:
    case JUNI_12:
        kenchiro_id_ = 1;
        PlaySound("textures/S20_Kitchomu_nr_nomisa_skip7.wav", NULL, SND_ASYNC);
        break;
    case JULI_29:
        PlaySound("textures/S23_Picasso03_nr_nomisa_skip5.5.wav", NULL, SND_ASYNC);
        kenchiro_id_ = 2;  // JULI_29,
        break;
    case AUGUST_15:
    case MAERZ_11_END:
        PlaySound("textures/S27_schwimmen03_nr_nomisa_skip14.wav", NULL, SND_ASYNC);
        kenchiro_id_ = 3;  // MAERZ_11_END,
        break;
    case UNKNOWN:
        PlaySound("textures/S29_Kobe02_nr_nomisa_skip6.5.wav", NULL, SND_ASYNC);
        kenchiro_id_ = 5;  // UNKNOWN
        break;
    case PROBERAUM:
        PlaySound("textures/S28_wasma02_nr_nomisa_skip7.wav", NULL, SND_ASYNC);
        kenchiro_id_ = 4;  // PROBERAUM
        break;
    }
}

void Zimmer::EndKenchiro(void) {
    PlaySound("textures/silence.wav", NULL, SND_ASYNC);
    draw_kenchiro_ = false;
}