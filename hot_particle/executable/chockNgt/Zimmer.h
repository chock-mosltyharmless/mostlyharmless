#pragma once
class Zimmer
{
public:
    Zimmer();
    ~Zimmer();

    int Draw(float time);

    void StartKenchiro(int id);
    void EndKenchiro(void);
    void StartLight(void) { has_light_ = true; }
    void EndLight(void) { has_light_ = false; }

private:
    // State machine
    bool draw_kenchiro_ = false;
    int kenchiro_id_ = 0;
    float kenchiro_start_time_ = 0.0f;
    float last_call_time_ = 0.0f;
    bool has_light_ = false;
    float brightness_ = 0.0f;
};

