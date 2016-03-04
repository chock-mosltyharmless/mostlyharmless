#pragma once
class Karaoke
{
public:
    Karaoke();
    ~Karaoke();

    void ToBeginning(void);
    int Draw(float time);

    void StartKenchiro(void);
    void EndKenchiro(void);

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    bool draw_kenchiro_ = true;
    float kenchiro_start_time_ = -100.0f;
};

