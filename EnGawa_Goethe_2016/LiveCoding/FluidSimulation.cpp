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
            fluid_amount_[next_][y][x] = 0.0f;
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
#if 0
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

        // Copy fluid to next frame
        for (int y = 0; y < kTotalHeight; y++) {
            for (int x = 0; x < kTotalWidth; x++) {
                fluid_amount_[next][y][x] = fluid_amount_[1 - next][y][x] + add_fluid;
                last_sum_fluid += fluid_amount_[next][y][x];
                // TODO: Max fluid speed 1.0?
            }
        }
#endif

        // Copy top row that is not copied otherwise
        for (int x = 0; x < kTotalWidth; x++) {
            fluid_amount_[next][0][x] = fluid_amount_[1 - next][0][x];
        }

        // Actually move the fluid
        float yp = -1.0f;
        float x_step = 2.0f / kTotalWidth;
        float y_step = 2.0f / kTotalHeight;
        for (int y = 0; y < kTotalHeight - 1; y++) {
            float xp = -1.0f;

            // Last row just copy
            fluid_amount_[next][y][kTotalWidth - 1] = fluid_amount_[1 - next][y][kTotalWidth - 1];

            for (int x = 0; x < kTotalWidth - 1; x++) {
                float current_amount = fluid_amount_[1 - next][y][x] + add_fluid;
                float right_amount = fluid_amount_[1 - next][y][x + 1];
                float bottom_amount = fluid_amount_[1 - next][y + 1][x];
                float next_amount = fluid_amount_[next][y][x];
                last_sum_fluid += current_amount;

                // Calculate velocities
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
                float right_velocity = -adjusted_xp * center_move * FS_FIELD_STRENGTH_CENTER;
                float down_velocity = -adjusted_yp * center_move * FS_FIELD_STRENGTH_CENTER;
                // Rotation
                right_velocity += FS_FIELD_STRENGTH_ROTATION * x_normal * (1.0f - fabsf(center_move));
                down_velocity += FS_FIELD_STRENGTH_ROTATION * y_normal * (1.0f - fabsf(center_move));

#if 1
                float pull_amount = CalculatePull(current_amount);
                float pull_right = CalculatePull(right_amount);
                float pull_bottom = CalculatePull(bottom_amount);
                right_velocity -= pull_amount - pull_right;
                down_velocity -= pull_amount - pull_bottom;
#else
                float pull_amount = CalculatePull(current_amount);
                right_velocity -= pull_amount;
                down_velocity -= pull_amount;
#endif

                if (right_velocity > 0.5f) right_velocity = 0.5f;
                if (down_velocity > 0.5f) down_velocity = 0.5f;

                // move right
                if (right_velocity > 0) {
                    float move_amount = right_velocity * current_amount;
                    next_amount -= move_amount;
                    fluid_amount_[next][y][x + 1] += move_amount;
                } else {
                    float move_amount = -right_velocity * right_amount;
                    next_amount += move_amount;
                    fluid_amount_[next][y][x + 1] -= move_amount;
                }

                // Copy y stuff
                float next_bottom = bottom_amount;
                // move bottom
                if (down_velocity > 0) {
                    float move_amount = down_velocity * current_amount;
                    next_amount -= move_amount;
                    next_bottom += move_amount;
                } else {
                    float move_amount = -down_velocity * bottom_amount;
                    next_amount += move_amount;
                    next_bottom -= move_amount;
                }
                fluid_amount_[next][y + 1][x] = next_bottom;

                // Check for invalid numbers. This thing is done last...
                if (isnan(next_amount) || next_amount < -0.5f || next_amount > 1.5f) {
                    next_amount = current_amount;
                }
                if (fabsf(next_amount) < 1e-7f) next_amount = 0.0f;

                fluid_amount_[next][y][x] = next_amount;

                xp += x_step;
            }

            yp += y_step;
        }

        // Check final row:
        for (int x = 0; x < kTotalWidth; x++) {
            int y = kTotalHeight - 1;
            float next_amount = fluid_amount_[next][y][x];
            if (isnan(next_amount) || next_amount < -0.5f || next_amount > 1.5f) {
                fluid_amount_[next][y][x] = fluid_amount_[1 - next][y][x];
            }
        }

        time_difference -= FS_UPDATE_STEP;
        next_ = 1 - next;
        if (time_difference > 0.2f) time_difference = 0.0f;
    }

    remain_time_ = time_difference;
}