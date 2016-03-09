#pragma once
class Smartphones
{
public:
    Smartphones();
    ~Smartphones();

    void ToBeginning(void);
    int Draw(float time);  // returns 1 if fade-out has finished
    void UpdateTime(float time) { last_call_time_ = time; }

    void TakeNextPicture();  // Next cow picture
    void NoMorePictures();  // Remove cow picture

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    int next_picture_id_ = 10;
    float last_picture_take_time_ = 0.0f;
    const static int kNumCowPictures = 8;
    bool show_cows_ = true;
    bool has_flashed_ = true;
};
