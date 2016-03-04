#pragma once
class Zimmer
{
public:
    Zimmer();
    ~Zimmer();

    void ToBeginning(void);
    int Draw(float time);

    void StartKenchiro(int id);
    void EndKenchiro(void);
    void StartLight(void) { has_light_ = true; }
    void EndLight(void) { has_light_ = false; }

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    bool draw_kenchiro_ = true;
    int kenchiro_id_ = 5000;
    float kenchiro_start_time_ = -100.0f;
    bool has_light_ = true;
    float brightness_ = 3.0f;
};

