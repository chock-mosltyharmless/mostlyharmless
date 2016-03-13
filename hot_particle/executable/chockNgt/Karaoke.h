#pragma once

enum KARAOKE_SCENE {
    TRENNUNG = 0,
    BAHNHOF_BAR,
    MITARBEITER,
    SEKUHARA
};

class Karaoke
{
public:
    Karaoke();
    ~Karaoke();

    void ToBeginning(void);
    int Draw(float time);
    void UpdateTime(float time) { last_call_time_ = time; }

    void StartScene(KARAOKE_SCENE scene) {
        has_white_fade_ = false;
        scene_ = scene;
        to_white_ = 1.0f;
    }
    void EndScene(void) {
        PlaySound("textures/silence.wav", NULL, SND_ASYNC);
        has_white_fade_ = true;
    }

    void StartKenchiro(void);
    void EndKenchiro(void);

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    bool draw_kenchiro_ = true;
    float kenchiro_start_time_ = -100.0f;
    KARAOKE_SCENE scene_ = TRENNUNG;
    int kenchiro_id_ = -3;
    float to_white_ = 3.0f;  // fade to white
    bool has_white_fade_ = true;
};

