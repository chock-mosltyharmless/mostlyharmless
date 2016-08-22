#pragma once

#define FS_UPDATE_STEP 0.005f
// Reduction of speed due to friction and so on
#define FS_VELOCITY_MULTIPLIER 0.f
#define FS_PULL_STRENGTH 0.15f
#define FS_PUSH_MULTIPLIER 0.5f
// Strength of the speed-up field
#define FS_FIELD_STRENGTH_CENTER 0.1f
#define FS_FIELD_STRENGTH_ROTATION 0.2f

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

private:
    const static int kWidth = 480;
    const static int kBorderWidth = 2;
    const static int kTotalWidth = kWidth + 2 * kBorderWidth;
    const static int kHeight = 240;
    const static int kBorderHeight = 2;
    const static int kTotalHeight = kHeight + 2 * kBorderWidth;

    float remain_time_;  // Time that wasn't used for update
    int next_;  // Buffer for next animation (0 or 1)
    float fluid_amount_[2][kTotalHeight][kTotalWidth];
    float fluid_velocity_[2][kTotalHeight][kTotalWidth][2];

    GLuint texture_id_;
};

