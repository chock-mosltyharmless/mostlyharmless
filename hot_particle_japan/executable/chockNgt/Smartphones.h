#pragma once

#include "chockNgt.h"
#include "Audio.h"

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
        char error_string[MAX_ERROR_LENGTH+1];
        scene_ = scene;
        has_white_fade_ = false;
        to_white_ = 2.0f;
        // Start Video imidiately.
        video_start_time_ = last_call_time_;
        current_panya_id_ = -1;
        panya_start_time_ = last_call_time_;

        // Play audio
        switch (scene) {
        case SM_KUHE:
            next_picture_id_ = kNumCowPictures - 1;
            for (int i = 0; i < 4; i++) {
                current_picture_id_[i] = kNumCowPictures - 1; // Go to beginning of pictures
                // turn back time so that the picture is already static
                last_picture_take_time_[i] = last_call_time_ - 10.0f;
            }
            TakeNextPicture();
            audio_.PlaySound("Kuhe_N4Y3R4.wav", 0, false, -1, error_string);
            subtitle_start_time_ = last_call_time_;
            subtitle_delay_ = 4.0f;
            subtitle_script_ = "Kuhe_N4Y3R4.txt";
            break;
        case SM_MINAMISOMA:
            audio_.PlaySound("Minamisoma.wav", 0, false, -1, error_string);
            subtitle_start_time_ = last_call_time_;
            subtitle_delay_ = 7.0f;
            subtitle_script_ = "Minamisoma.txt";
            break;
        default:  // This is a bug
            audio_.StopSound(0, 36.0f, error_string);
            break;
        }
    }

    void EndScene(void) {
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.StopSound(0, 36.0f, error_string);
        has_white_fade_ = true;
    }

    void TakeNextPicture(int position = 0);  // Next picture, 0 center; 1 left; 2 bottom; 3 right
    void NoMorePictures();  // Remove cow picture

    void NextPanya() {
        // I don't care about panya ID because there is only one
        // per scene
        char error_string[MAX_ERROR_LENGTH+1];
        audio_.PlaySound("panya_klingelton.wav", 1, false, -1, error_string);
        current_panya_id_ = 1;
        panya_start_time_ = last_call_time_;
    }

private:
    // State machine (initialized incorrectly to test toBeginning()
    float last_call_time_ = 0.0f;
    SMARTPHONE_SCENE scene_ = SM_KUHE;
    bool has_white_fade_ = false;
    float to_white_ = 0.0f;
    int next_picture_id_ = 10;
    int current_picture_id_[4] = { 10, 10, 10, 10 };
    float last_picture_take_time_[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const static int kNumCowPictures = 8;
    bool show_pictures_[4] = { true, true, true, true };
    bool has_flashed_[4] = { true, true, true, true };
    float video_start_time_ = 0.0f;
    int current_panya_id_ = -1;
    float panya_start_time_ = 0.0f;
};
