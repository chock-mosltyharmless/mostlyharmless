#pragma once

class Cafe
{
public:
    Cafe();
    ~Cafe();

    void ToBeginning(void);
    int Draw(float time);
    void UpdateTime(float time) { last_call_time_ = time; }

    void StartScene(int nothing) {
        nothing = nothing;
        has_white_fade_ = false;
        to_white_ = 1.0f;
    }
    void EndScene(void) {
        has_white_fade_ = true;
    }

    void StartVideo(void) {
        draw_video_ = true;
        video_start_time_ = last_call_time_;
        PlaySound("textures/Sawa_5.wav", NULL, SND_ASYNC);
    }
    void EndVideo(void) {
        draw_video_ = false;
        PlaySound("textures/silence.wav", NULL, SND_ASYNC);
    }

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    float to_white_ = 3.0f;  // fade to white
    bool draw_video_ = true;
    float video_start_time_ = 0.0f;
    bool has_white_fade_ = true;
};

