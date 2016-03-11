#pragma once

enum CAR_SCENE {
    BEGRUSSUNG = 0,

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
        to_white_ = 1.0f;
        // Start Video imidiately.
        video_start_time_ = last_call_time_;
    }
    void EndScene(void) {
        has_white_fade_ = true;
    }

private:
    float last_call_time_ = 0.0f;
    CAR_SCENE scene_ = BEGRUSSUNG;
    float to_white_ = 3.0f;  // fade to white
    bool draw_video_ = true;
    float video_start_time_ = 0.0f;
    bool has_white_fade_ = true;
};

