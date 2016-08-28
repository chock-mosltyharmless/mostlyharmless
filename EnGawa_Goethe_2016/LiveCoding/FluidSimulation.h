#pragma once

#define FS_UPDATE_STEP 0.01f
// Reduction of speed due to friction and so on
#define FS_PULL_STRENGTH 0.1f
#define FS_PUSH_MULTIPLIER 0.5f
// Strength of the speed-up field
#define FS_FIELD_STRENGTH_CENTER 0.4f
#define FS_FIELD_STRENGTH_ROTATION 0.2f
#define FS_TOTAL_SUM_FLUID (120*40)
#define FS_HILL_ROTATION_SPEED 0.2f

class FluidSimulation
{
public:
    FluidSimulation();
    virtual ~FluidSimulation();

    // To create texture, field and so on
    void Init(void);

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

    void DrawLine(float startX, float startY, float dirX, float dirY, int length, int buffer);

    static float frand(void);

    bool request_set_points_;

private:
    const static int kWidth = 320;
    const static int kBorderWidth = 2;
    const static int kTotalWidth = kWidth + 2 * kBorderWidth;
    const static int kHeight = 180;
    const static int kBorderHeight = 2;
    const static int kTotalHeight = kHeight + 2 * kBorderWidth;

    double current_time;

    float last_sum_fluid;
    float remain_time_;  // Time that wasn't used for update
    int next_;  // Buffer for next animation (0 or 1)
    float fluid_amount_[2][kTotalHeight][kTotalWidth];

    GLuint texture_id_;
};

