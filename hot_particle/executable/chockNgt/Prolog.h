#pragma once
class Prolog
{
public:
    Prolog();
    ~Prolog();

    void ToBeginning(void);
    int Draw(float time);

    void StartVideo(void);
    void EndVideo(void) { show_video_ = false; }
    void StartLight(void) { has_light_ = true; }
    void EndLight(void) { has_light_ = false; }

private:
    // State machine (wrong init to check ToBeginning)
    float last_call_time_ = 0.0f;
    bool show_video_ = true;
    float video_start_time_ = 0.0f;
    float brightness_ = 1.0f;
    bool has_light_ = true;
};

