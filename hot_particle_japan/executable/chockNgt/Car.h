#pragma once

#include "chockNgt.h"
#include "Audio.h"

#define FAHRT_SOUND_VOLUME 0.1f

enum CAR_SCENE {
    BEGRUSSUNG = 0,  // Automatically moves on to Zahnarzt
    TOMOBE,
    SIEVERT,  // Automatically moves on to Polizei
    TAMURA,
    KATSURAO13, // Automatically moves on to Kuhe
    KATSURAO14, // Automatically moves on to Wohin
    // Minnamisoma mache ich direkt mit Smartphones.
    ABSCHIED,
    ZAHNARZT,
    POLIZEI,
    KUHE,
    WOHIN,
    END_IT,  // no following scene
};

class Car
{
public:
    Car();
    ~Car();

    void ToBeginning(void);
    int Draw(float time);
    void UpdateTime(float time) { last_call_time_ = time; }

    void StartScene(CAR_SCENE scene) {
        scene_ = scene;
        has_white_fade_ = false;
        char error_string[MAX_ERROR_LENGTH+1];
        switch (scene) {
        case BEGRUSSUNG:
            to_white_ = 1.5f;
            break;
        case ABSCHIED:
            to_white_ = 1.0f;
            break;
        default:
            to_white_ = 2.0f;
            break;
        }
        // Start Video imidiately.
        video_start_time_ = last_call_time_;
        next_scene_ = END_IT;

        // Play audio
        switch (scene) {
        case BEGRUSSUNG:
            audio_.PlaySound("Begrussung_N1Y1R3.wav", 0, false, -1, error_string);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "Begrussung_N1Y1R3.txt";
            break;
        case TOMOBE:
            audio_.StopSound(0, 36.0f, error_string);
            break;
        case SIEVERT:
            audio_.PlaySound("Sievert_N1Y1R1.wav", 0, false, -1, error_string);
            audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "Sievert_N1Y1R1.txt";
            break;
        case TAMURA:
            audio_.PlaySound("Tamura_N3Y1R1.wav", 0, false, -1, error_string);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "Tamura_N3Y1R1.txt";
            break;
        case KATSURAO13:
            audio_.PlaySound("13Katsurao_N1Y3R3.wav", 0, false, -1, error_string);
            audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "13Katsurao_N1Y3R3.txt";
            break;
        case KATSURAO14:
            audio_.PlaySound("14Katsurao_N1Y4R1.wav", 0, false, -1, error_string);
            audio_.PlaySound("fahrt.wav", 2, true, 24.0f, error_string, FAHRT_SOUND_VOLUME);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "14Katsurao_N1Y4R1.txt";
            break;
        case ABSCHIED:
            audio_.PlaySound("Abschied_N1Y1R2.wav", 0, false, -1, error_string);
            subtitle_start_time_ = last_call_time_;
            subtitle_script_ = "Abschied_N1Y1R2.txt";
            break;
        default:  // This is a bug
            audio_.StopSound(0, 36.0f, error_string);
            break;
        }
        current_panya_id_ = -1;
        panya_start_time_ = last_call_time_;
    }

    void EndScene(void) {
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.StopSound(0, 36.0f, error_string);
        if (scene_ != SIEVERT) {
            audio_.StopSound(2, 36.0f, error_string);
        }
        has_white_fade_ = true;
        next_scene_ = END_IT;
        current_panya_id_ = -1;
        panya_start_time_ = last_call_time_;
    }

    void NextPanya() {
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.PlaySound("panya_klingelton.wav", 1, false, -1, error_string);
        current_panya_id_++;
        panya_start_time_ = last_call_time_;
    }

private:
    float last_call_time_ = 0.0f;
    CAR_SCENE scene_ = BEGRUSSUNG;
    CAR_SCENE next_scene_ = END_IT;
    float to_white_ = 3.0f;  // fade to white
    bool draw_video_ = true;
    float video_start_time_ = 0.0f;
    bool has_white_fade_ = true;
    bool show_gps_ = true;
    float gps_start_time_ = 0.0f;

    // Panya
    int current_panya_id_ = -1;
    float panya_start_time_ = 0.0f;
};

