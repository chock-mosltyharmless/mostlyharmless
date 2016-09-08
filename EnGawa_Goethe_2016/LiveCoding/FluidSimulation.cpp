#include "stdafx.h"
#include "FluidSimulation.h"
#include "glext.h"
#include "Parameter.h"
#include <math.h>


extern float interpolatedParameters[];
extern float aspectRatio;

FluidSimulation::FluidSimulation() {
    next_ = 0;
    request_set_points_ = false;
    //last_music_beat_ = 1.0e10f;  // Don't start the music
    last_music_beat_ = 0.0f;  // Start right off
}

FluidSimulation::~FluidSimulation() {
}

float FluidSimulation::frand(void) {
    int randint = rand() % RAND_MAX;
    return (float)randint / (float)(RAND_MAX);
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
    current_time = 0.0;

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
    return texture_id_;
}

void FluidSimulation::DrawLine(float startX, float startY, float dirX, float dirY,
                               int length, int buffer) {
    int xp = (int)(startX * kTotalWidth + kTotalWidth) * 128;
    int yp = (int)(startY * kTotalHeight + kTotalHeight) * 128;
    int dx = (int)(dirX * 256);
    int dy = (int)(dirY * 256);

    for (int i = 0; i < length; i++) {
        int xpos = xp / 256;
        int ypos = yp / 256;
        if (xpos >= 0 && xpos < kTotalWidth &&
            ypos >= 0 && ypos < kTotalHeight) {
            fluid_amount_[buffer][ypos][xpos] = 1.0f;
        }
        xp += dx;
        yp += dy;
    }
}

void FluidSimulation::SetPoints(void) {
    int next = next_;
    int current = 1 - next;
    float time = (float)current_time;
    for (int y = 0; y < kTotalHeight; y++) {
        for (int x = 0; x < kTotalWidth; x++) {
            fluid_amount_[current][y][x] = 0.0f;
        }
    }

    int length = (int)(interpolatedParameters[4] * 40) + 5;
    const int kNumDots = FS_TOTAL_SUM_FLUID / length;

    for (int dot = 0; dot < kNumDots; dot++) {
        float angle = frand() * 3.1415f * 2.0f;
        float distance = 2.0f * frand() - 1.0f;
        distance = distance * distance * distance;
        distance *= 0.3f * interpolatedParameters[6];
        distance += 0.5f;
        float direction = (interpolatedParameters[5] *  4.0f * (frand() - 0.5f)) +
            angle - 3.15415f/2.0f;
#if 0
        int xp = (int)(distance * sin(angle) * kTotalWidth / 2) + kTotalWidth / 2;
        int yp = (int)(distance * cos(angle) * kTotalHeight / 2) + kTotalHeight / 2;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int xpos = xp + dx;
                int ypos = yp + dy;
                if (xpos >= 0 && xpos < kTotalWidth &&
                    ypos >= 0 && ypos < kTotalHeight) {
                    // Should always be true...
                    fluid_amount_[current][ypos][xpos] = 1.0f;
                }
            }
        }
#else
        DrawLine(distance * sinf(angle), distance * cosf(angle),
            sinf(direction), cosf(direction),
            length, current);
#endif
    }
}

void FluidSimulation::SetRegularLines(void) {
    int next = next_;
    int current = 1 - next;
    float time = (float)current_time;
    for (int y = 0; y < kTotalHeight; y++) {
        for (int x = 0; x < kTotalWidth; x++) {
            fluid_amount_[current][y][x] = 0.0f;
        }
    }

    float line_length = interpolatedParameters[6];
    line_length = -0.5f * cosf(time * 0.65f) + 1.f;

    int length = (int)(line_length * 60) + 5;
    const int kNumLines = FS_TOTAL_SUM_FLUID / length;

    float diff_dir = interpolatedParameters[5];
    diff_dir = 2.0f * sinf(time * 0.25f);
    if (diff_dir < 0.0f) diff_dir = 0.0f;
    if (time < 27.0f) diff_dir = 0.0f;

    float spread = interpolatedParameters[6];
    spread = -sinf(time * 0.3f);
    if (spread < -.5f) spread = -0.5f;
    if (time < 13.8f) spread = 0.0f;

    for (int dot = 0; dot < kNumLines; dot++) {
        float angle = (float)dot * 3.1415f * 2.0f / kNumLines +
            time * 0.3f;
        float distance = 0.5f - (float)(dot & 1);
        distance *= 0.2f * spread;
        distance += 0.5f;
        //float direction = (interpolatedParameters[5] * 4.0f * (frand() - 0.5f)) +
        //    angle - 3.15415f / 2.0f;
        float direction = angle - 3.15415f / 2.0f;
        float x_start = distance * sinf(angle);
        float y_start = distance * cosf(angle);
        float x_dir = sinf(direction);
        float y_dir = cosf(direction);
        x_start -= 2.0f * diff_dir * x_dir * length * (float)(dot & 1) / (float)kTotalWidth;
        y_start -= 2.0f * diff_dir * y_dir * length * (float)(dot & 1) / (float)kTotalHeight;
        DrawLine(x_start, y_start, x_dir, y_dir, length, current);
    }
}

void FluidSimulation::UpdateTime(float time_difference) {
    time_difference += remain_time_;
    remain_time_ = 0.0f;
    bool did_update = false;

    while (time_difference > FS_UPDATE_STEP) {
        if (request_set_points_) {
            SetRegularLines();
            request_set_points_ = false;
            last_music_beat_ = (float)current_time;
        }

        if (current_time - last_music_beat_ > FS_MUSIC_BEAT &&
            current_time < 159.0f) {
            request_set_points_ = true;
        }

        current_time += FS_UPDATE_STEP;
        float time = (float)current_time;
        did_update = true;
        int next = next_;
        float add_fluid = FS_TOTAL_SUM_FLUID - last_sum_fluid;
        add_fluid /= kTotalHeight * kTotalWidth;
        last_sum_fluid = 0.0f;

        // Copy top row that is not copied otherwise
        for (int x = 0; x < kTotalWidth; x++) {
            fluid_amount_[next][0][x] = fluid_amount_[1 - next][0][x];
        }

        // Middle hill precalculation
        float hill_rotation = time * FS_HILL_ROTATION_SPEED;
        float sinf_hill_rotation = sinf(hill_rotation);
        float cosf_hill_rotation = cosf(hill_rotation);
        float sinf_inv_hill_rotation = sinf(-hill_rotation);
        float cosf_inv_hill_rotation = cosf(-hill_rotation);

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
                float length = sqrtf(xp * xp + yp * yp);
                if (length < 0.0001f) length = 0.0001f;
                float x_normal = yp / length;
                float y_normal = -xp / length;
                // Circle-pos
                float center_move = length - 0.5f;
                float right_velocity = -xp * center_move * FS_FIELD_STRENGTH_CENTER;
                float down_velocity = -yp * center_move * FS_FIELD_STRENGTH_CENTER;
                // Rotation
                right_velocity += FS_FIELD_STRENGTH_ROTATION * x_normal * (1.0f - fabsf(center_move));
                down_velocity += FS_FIELD_STRENGTH_ROTATION * y_normal * (1.0f - fabsf(center_move));

                // Some middle hill
                float hill_rotation = time * FS_HILL_ROTATION_SPEED;
                //float xp_rot = cos(hill_rotation) * xp - sin(hill_rotation) * yp;
                float yp_rot = sinf_hill_rotation * xp + cosf_hill_rotation * yp;
                float line_dist = yp_rot * yp_rot;
                float hill_amount = interpolatedParameters[2] * 0.03f / (line_dist + 0.03f);
                float away_amount = 1.0f * yp_rot * hill_amount;  // goes to linear?
                right_velocity += -sinf_inv_hill_rotation * away_amount;
                down_velocity += cosf_inv_hill_rotation * away_amount;

#if 1
                // Tear up
                right_velocity += 0.3f * sinf(yp * 9.f + time) *
                    cosf(xp * (8.f + sinf(time * 0.3f)) - time * 0.7f) * interpolatedParameters[3];
                down_velocity += 0.3f * cosf(yp * (7.f - cosf(time * 0.27f)) - time * 0.9f) *
                    sinf(xp * 5.f + time * 0.6f) * interpolatedParameters[3];
#endif

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
                float move_amount;
                if (right_velocity > 0) {
                    move_amount = right_velocity * current_amount;
                } else {
                    move_amount = right_velocity * right_amount;
                }
                next_amount -= move_amount;
                fluid_amount_[next][y][x + 1] += move_amount;
                // Copy y stuff
                float next_bottom = bottom_amount;
                // move bottom
                if (down_velocity > 0) {
                    move_amount = down_velocity * current_amount;
                } else {
                    move_amount = down_velocity * bottom_amount;
                }
                next_amount -= move_amount;
                next_bottom += move_amount;
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

    if (did_update) {
        glBindTexture(GL_TEXTURE_2D, texture_id_);
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            0, 0, kTotalWidth, kTotalHeight,
            GL_RED, GL_FLOAT, fluid_amount_[1 - next_]);
    }

    remain_time_ = time_difference;
}