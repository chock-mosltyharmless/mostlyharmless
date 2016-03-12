#pragma once

enum SMARTPHONE_SCENE {
    SM_KUHE = 0,
    SM_MINAMISOMA
};

class Smartphones
{
public:
    Smartphones();
    ~Smartphones();

    void ToBeginning(void);
    int Draw(float time);  // returns 1 if fade-out has finished
    void UpdateTime(float time) { last_call_time_ = time; }

    void StartScene(SMARTPHONE_SCENE scene) {
        scene_ = scene;
        has_white_fade_ = false;
        to_white_ = 1.0f;
        // Start Video imidiately.
        video_start_time_ = last_call_time_;

        // Play audio
        switch (scene) {
        case SM_KUHE:
            next_picture_id_ = 10; // Go to beginning of pictures
            TakeNextPicture();
            // turn back time so that the picture is already static
            last_picture_take_time_ = last_call_time_ - 10.0f;
            PlaySound("textures/Kuhe_N4Y3R4.wav", NULL, SND_ASYNC);
            break;
        case SM_MINAMISOMA:
            PlaySound("textures/minamisoma.wav", NULL, SND_ASYNC); // I need it...
            break;
        default:  // This is a bug
            PlaySound("textures/silence.wav", NULL, SND_ASYNC);
            break;
        }
    }

    void EndScene(void) {
        has_white_fade_ = true;
    }


    void TakeNextPicture();  // Next cow picture
    void NoMorePictures();  // Remove cow picture

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    SMARTPHONE_SCENE scene_ = SM_KUHE;
    bool has_white_fade_ = false;
    float video_start_time_;
    float to_white_ = 0.0f;
    int next_picture_id_ = 10;
    float last_picture_take_time_ = 0.0f;
    const static int kNumCowPictures = 8;
    bool show_pictures_ = true;
    bool has_flashed_ = true;
};
