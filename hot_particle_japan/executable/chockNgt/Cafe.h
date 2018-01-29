#pragma once

#include "chockNgt.h"
#include "Audio.h"

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
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.StopSound(0, 36.0f, error_string);
        has_white_fade_ = true;
    }

    void StartVideo(void) {
        draw_video_ = true;
        video_start_time_ = last_call_time_;
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.PlaySound("Sawa_5.wav", 0, false, -1, error_string);
        subtitle_start_time_ = last_call_time_;
        subtitle_delay_ = 8.0f;
        subtitle_script_ = "Sawa_5.txt";
    }
    void EndVideo(void) {
        draw_video_ = false;
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.StopSound(0, 36.0f, error_string);
    }

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    float to_white_ = 3.0f;  // fade to white
    bool draw_video_ = true;
    float video_start_time_ = 0.0f;
    bool has_white_fade_ = true;
};

