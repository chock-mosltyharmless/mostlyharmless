#pragma once

#define FS_UPDATE_STEP 0.01f
// Reduction of speed due to friction and so on
#define FS_VELOCITY_MULTIPLIER 0.9f
#define FS_PULL_STRENGTH 0.01f
#define FS_PUSH_MULTIPLIER 1.0f

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
    const static int kWidth = 160;
    const static int kBorderWidth = 20;
    const static int kTotalWidth = kWidth + 2 * kBorderWidth;
    const static int kHeight = 90;
    const static int kBorderHeight = 20;
    const static int kTotalHeight = kHeight + 2 * kBorderWidth;

    float remain_time_;  // Time that wasn't used for update
    int next_;  // Buffer for next animation (0 or 1)
    float fluid_amount_[2][kTotalHeight][kTotalWidth];
    float fluid_velocity_[2][kTotalHeight][kTotalWidth][2];

    GLuint texture_id_;
};

