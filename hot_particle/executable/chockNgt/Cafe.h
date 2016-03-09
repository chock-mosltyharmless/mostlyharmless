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

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    float to_white_ = 3.0f;  // fade to white
    bool has_white_fade_ = true;
};

