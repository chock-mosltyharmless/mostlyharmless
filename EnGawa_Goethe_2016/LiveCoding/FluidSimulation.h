#pragma once

#include "Configuration.h"

#define FS_UPDATE_STEP 0.01f
// Reduction of speed due to friction and so on
#define FS_PULL_STRENGTH 0.1f
#define FS_PUSH_MULTIPLIER 0.5f
// Strength of the speed-up field
#define FS_FIELD_STRENGTH_CENTER 0.4f
#define FS_FIELD_STRENGTH_ROTATION 0.2f
#define FS_TOTAL_SUM_FLUID (120*40)
#define FS_HILL_ROTATION_SPEED 0.2f

#define FS_MUSIC_BEAT 0.405f

class FluidSimulation
{
public:
    FluidSimulation();
    virtual ~FluidSimulation();

    // To create texture, field and so on
    void Init(bool show_stuff);

    void UpdateTime(float time_difference);
    
    // For debugging only? Create a texture for drawing the field
    GLuint GetTexture(void);

    // Calculate pull derived from fill (0.0 to 1.0)
    static float CalculatePull(float amount) {
        float pull_amount = amount - amount * amount;
        if (amount > 1.0f) pull_amount = (1.0f - amount) * FS_PUSH_MULTIPLIER;
        //if (amount < 0.0f) pull_amount = -amount;  // Should not happen, still negative fluid --> pull
        if (amount < 0.0f) pull_amount = amount;
        return pull_amount * FS_PULL_STRENGTH;
    }

    void SetPoints(void);
    void SetRegularLines(void);

    void DrawLine(float startX, float startY, float dirX, float dirY, int length, int buffer);

    static float frand(void);

    bool request_set_points_;

    unsigned char *GetBackBuffer() {
        return back_buffer_[0][0];
    }

    void SetBackBuffer() {
        float *dest = fluid_amount_[1 - next_][0];
        unsigned char(*source)[4] = back_buffer_[0];
        for (int x = 0; x < kTotalHeight * kTotalWidth; x++) {
            dest[x] = (float)(255 - source[x][1]) / 255.0f;
        }
    }

    void PushApart(void) {
        push_apart_ = true;
    }

private:
    const static int kWidth = X_HIGHLIGHT;
    const static int kBorderWidth = 0;
    const static int kTotalWidth = kWidth + 2 * kBorderWidth;
    const static int kHeight = Y_HIGHLIGHT;
    const static int kBorderHeight = 0;
    const static int kTotalHeight = kHeight + 2 * kBorderWidth;

    bool show_stuff_;
    bool push_apart_;

    double current_time;

    float last_sum_fluid;
    float remain_time_;  // Time that wasn't used for update
    int next_;  // Buffer for next animation (0 or 1)
    float fluid_amount_[2][kTotalHeight][kTotalWidth];
    unsigned char back_buffer_[kTotalHeight][kTotalWidth][4];

    // Some stuff I will probably not use for long
    float last_music_beat_;

    GLuint texture_id_;
};

