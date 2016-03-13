#pragma once

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
        to_white_ = 2.0f;
        // Start Video imidiately.
        video_start_time_ = last_call_time_;
        next_scene_ = END_IT;

        // Play audio
        switch (scene) {
        case BEGRUSSUNG:
            PlaySound("textures/Begrussung_N1Y1R3.wav", NULL, SND_ASYNC);
            break;
        case TOMOBE:
            PlaySound("textures/silence.wav", NULL, SND_ASYNC);
            break;
        case SIEVERT:
            PlaySound("textures/Sievert_N1Y1R1.wav", NULL, SND_ASYNC);
            break;
        case TAMURA:
            PlaySound("textures/Tamura_N3Y1R1.wav", NULL, SND_ASYNC);
            break;
        case KATSURAO13:
            PlaySound("textures/13Katsurao_N1Y3R3.wav", NULL, SND_ASYNC);
            break;
        case KATSURAO14:
            PlaySound("textures/14Katsurao_N1Y4R1.wav", NULL, SND_ASYNC);
            break;
        case ABSCHIED:
            PlaySound("textures/Abschied_N1Y1R2.wav", NULL, SND_ASYNC);
            break;
        default:  // This is a bug
            PlaySound("textures/silence.wav", NULL, SND_ASYNC);
            break;
        }
    }
    void EndScene(void) {
        has_white_fade_ = true;
        next_scene_ = END_IT;
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
};

