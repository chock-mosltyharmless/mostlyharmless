#include "stdafx.h"
#include "FluidSimulation.h"
#include "glext.h"
#include "Parameter.h"
#include <math.h>

extern float interpolatedParameters[];

FluidSimulation::FluidSimulation() {
    next_ = 0;
}

FluidSimulation::~FluidSimulation() {
}

void FluidSimulation::Init(void) {
    next_ = 1;
    for (int y = 0; y < kTotalHeight; y++) {
        for (int x = 0; x < kTotalWidth; x++) {
            fluid_amount_[1 - next_][y][x] = 0.0f;
            fluid_velocity_[1 - next_][y][x][0] = 0.0f;
            fluid_velocity_[1 - next_][y][x][1] = 0.0f;
        }
    }
    remain_time_ = 0.0f;

    // Put some fluid to the left and right
#if 1
    last_sum_fluid = 0.0f;
    for (int y = kTotalHeight / 2 - 20; y < kTotalHeight / 2 + 21; y++) {
        for (int x = 0; x < 80; x++) {
            fluid_amount_[1 - next_][y + 20][x] = 1.0f;
            fluid_amount_[1 - next_][y - 20][kTotalWidth - x - 1] = 1.0f;
            last_sum_fluid += 2.0f;
        }
    }
#else
    last_sum_fluid = 0.0f;
    float cell_fluid = FS_TOTAL_SUM_FLUID / (kTotalHeight * kTotalWidth);
    for (int y = 0; y < kTotalHeight; y++) {
        for (int x = 0; x < kTotalHeight; x++) {
            last_sum_fluid += cell_fluid;
            fluid_amount_[1 - next_][y][x] = cell_fluid;
        }
    }
#endif

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
        kTotalWidth, kTotalHeight,
        0, GL_RED, GL_FLOAT, fluid_amount_[1 - next_]);
}

GLuint FluidSimulation::GetTexture(void) {
    // Send data to GPU
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
            0, 0, kTotalWidth, kTotalHeight,
        GL_RED, GL_FLOAT, fluid_amount_[1 - next_]);
    return texture_id_;
}

void FluidSimulation::UpdateTime(float time_difference) {
    time_difference += remain_time_;
    remain_time_ = 0.0f;

    while (time_difference > FS_UPDATE_STEP) {
        int next = next_;
        float add_fluid = FS_TOTAL_SUM_FLUID - last_sum_fluid;
        add_fluid /= kTotalHeight * kTotalWidth;
        last_sum_fluid = 0.0f;
#if 1
        // Reduce speed due to friction
        for (int y = 0; y < kTotalHeight; y++) {
            for (int x = 0; x < kTotalWidth; x++) {
                fluid_velocity_[1 - next][y][x][0] *= FS_VELOCITY_MULTIPLIER;
                if (fabsf(fluid_velocity_[1 - next][y][x][0]) < 1.e-5f) fluid_velocity_[1 - next][y][x][0] = 0.0f;
                fluid_velocity_[1 - next][y][x][1] *= FS_VELOCITY_MULTIPLIER;
                if (fabsf(fluid_velocity_[1 - next][y][x][1]) < 1.e-5f) fluid_velocity_[1 - next][y][x][0] = 0.0f;
            }
        }

        // Do fluid - pull
        for (int y = 1; y < kTotalHeight - 1; y++) {
            for (int x = 1; x < kTotalWidth - 1; x++) {
                float amount = fluid_amount_[1 - next][y][x];
                float pull_amount = amount - amount * amount;
                if (amount > 1.0f) pull_amount = (1.0f - amount) * FS_PUSH_MULTIPLIER;
                if (amount < 0.0f) pull_amount = -amount;  // Should not happen, still negative fluid --> pull
                pull_amount *= FS_PULL_STRENGTH;
                //if (pull_amount > 0.5f) pull_amount = 0.5f;
                fluid_velocity_[1 - next][y][x + 1][0] -= pull_amount;
                fluid_velocity_[1 - next][y][x - 1][0] += pull_amount;
                fluid_velocity_[1 - next][y + 1][x][1] -= pull_amount;
                fluid_velocity_[1 - next][y - 1][x][1] += pull_amount;
            }
        }

        // Apply trace field
        float yp = -1.0f;
        float x_step = 2.0f / kTotalWidth;
        float y_step = 2.0f / kTotalHeight;
        for (int y = 0; y < kTotalHeight; y++) {
            float xp = -1.0f;
            for (int x = 0; x < kTotalWidth; x++) {
                float adjusted_xp = xp;
                float adjusted_yp = yp;

                adjusted_xp += interpolatedParameters[2] * fabsf(yp) * 0.75f;
                adjusted_xp -= interpolatedParameters[3] * fabsf(yp) * 0.75f;
                adjusted_yp += interpolatedParameters[3] * xp * 0.25f;

                float length = sqrtf(adjusted_xp * adjusted_xp + adjusted_yp * adjusted_yp);
                if (length < 0.0001f) length = 0.0001f;
                float x_normal = adjusted_yp / length;
                float y_normal = -adjusted_xp / length;

                // Circle-pos
                float center_move = length - 0.5f;

                fluid_velocity_[1 - next][y][x][0] -= adjusted_xp * center_move * FS_FIELD_STRENGTH_CENTER;
                fluid_velocity_[1 - next][y][x][1] -= adjusted_yp * center_move * FS_FIELD_STRENGTH_CENTER;

                // Rotation
                fluid_velocity_[1 - next][y][x][0] += FS_FIELD_STRENGTH_ROTATION * x_normal * (1.0f - fabsf(center_move));
                fluid_velocity_[1 - next][y][x][1] += FS_FIELD_STRENGTH_ROTATION * y_normal * (1.0f - fabsf(center_move));

                // Move out of middle
                //fluid_velocity_[1 - next][y][x][0] += 0.01f / (length * length + 0.1f);

                xp += x_step;
            }
            yp += y_step;
        }
#endif

        // Copy fluid to next frame
        for (int y = 0; y < kTotalHeight; y++) {
            for (int x = 0; x < kTotalWidth; x++) {
                fluid_amount_[next][y][x] = fluid_amount_[1 - next][y][x] + add_fluid;
                last_sum_fluid += fluid_amount_[next][y][x];
                // TODO: Max fluid speed 1.0?
            }
        }

        // Actually move the fluid
#if 0
        float yp = -1.0f;
        float x_step = 2.0f / kTotalWidth;
        float y_step = 2.0f / kTotalHeight;
#endif
        for (int y = 1; y < kTotalHeight - 1; y++) {
            float xp = -1.0f;
            for (int x = 1; x < kTotalWidth - 1; x++) {
#if 1
                // Update velocity
                fluid_velocity_[next][y][x][0] = fluid_velocity_[1 - next][y][x][0];
                fluid_velocity_[next][y][x][1] = fluid_velocity_[1 - next][y][x][1];
                float left_right_velocity = fluid_velocity_[1 - next][y][x][0];
                float top_bottom_velocity = fluid_velocity_[1 - next][y][x][1];
#else
                float length = sqrtf(xp * xp + yp * yp);
                if (length < 0.0001f) length = 0.0001f;
                float x_normal = yp / length;
                float y_normal = -xp / length;

                // Circle-pos
                float center_move = length - 0.5f;
                float left_right_velocity = -xp * center_move * FS_FIELD_STRENGTH_CENTER;
                float top_bottom_velocity = -yp * center_move * FS_FIELD_STRENGTH_CENTER;

                // Rotation
                left_right_velocity += FS_FIELD_STRENGTH_ROTATION * x_normal * (1.0f - fabsf(center_move));
                top_bottom_velocity += FS_FIELD_STRENGTH_ROTATION * y_normal * (1.0f - fabsf(center_move));
#endif

                // from left
                float move_amount;
                float left_velocity = left_right_velocity > 0.0f ? left_right_velocity : 0.0f;
                move_amount = left_velocity * fluid_amount_[1 - next][y][x - 1];
                fluid_amount_[next][y][x - 1] -= move_amount;
                fluid_amount_[next][y][x] += move_amount;
                // from right
                float right_velocity = left_right_velocity < 0.0f ? left_right_velocity : 0.0f;
                move_amount = -right_velocity * fluid_amount_[1 - next][y][x + 1];
                fluid_amount_[next][y][x + 1] -= move_amount;
                fluid_amount_[next][y][x] += move_amount;

                // from bottom/top
                float top_velocity = top_bottom_velocity > 0.0f ? top_bottom_velocity : 0.0f;
                move_amount = top_velocity * fluid_amount_[1 - next][y - 1][x];
                fluid_amount_[next][y - 1][x] -= move_amount;
                fluid_amount_[next][y][x] += move_amount;
                float bottom_velocity = top_bottom_velocity < 0.0f ? top_bottom_velocity : 0.0f;
                move_amount = -bottom_velocity * fluid_amount_[1 - next][y + 1][x];
                fluid_amount_[next][y + 1][x] -= move_amount;
                fluid_amount_[next][y][x] += move_amount;

                // Denormals
                if (fabsf(fluid_amount_[next][y][x]) < 1.e-8f) fluid_amount_[next][y][x] = 0.0f;

                // Check for invalid numbers
                if (isnan(fluid_amount_[next][y][x]) ||
                    fluid_amount_[next][y][x] < -1.0f ||
                    fluid_amount_[next][y][x] > 2.0f) {
                    fluid_amount_[next][y][x] = fluid_amount_[1 - next][y][x];
                }

                xp += x_step;
            }
            yp += y_step;
        }

        time_difference -= FS_UPDATE_STEP;
        next_ = 1 - next;
        if (time_difference > 0.2f) time_difference = 0.0f;
    }

    remain_time_ = time_difference;
}