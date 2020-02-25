#include "Fractal.h"

#pragma comment(lib,"winmm.lib")

#include "../imgui/imgui.h"

#include <memory>

#include <Windows.h>
#include <math.h>
#include <memory.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>

#define TWO_PI (2.0f * 3.1415f)
#define MZK_RATE 44100
#define MZK_NUMCHANNELS 1
// 5 second audio
#define MZK_NUMSAMPLES (5 * MZK_RATE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES * MZK_NUMCHANNELS)

static short music[MZK_NUMSAMPLESC + 22];
static const int kWavHeader_[11] = {
    0x46464952,
    MZK_NUMSAMPLESC * sizeof(short) + 36,
    0x45564157,
    0x20746D66,
    16,
    WAVE_FORMAT_PCM | (MZK_NUMCHANNELS << 16),
    MZK_RATE,
    MZK_RATE*MZK_NUMCHANNELS * sizeof(short),
    (MZK_NUMCHANNELS * sizeof(short)) | ((8 * sizeof(short)) << 16),
    0x61746164,
    MZK_NUMSAMPLESC * sizeof(short)
};

Matrix2x3::Matrix2x3(void)
{
    memset(a_, 0, sizeof(a_));
    a_[0][0] = 1.0f;
    a_[1][1] = 1.0f;
}

Matrix2x3::~Matrix2x3()
{
}

void Matrix2x3::Set(float radians, float scale_x, float scale_y, float trans_x, float trans_y)
{
    Matrix2x3 scale;
    scale.ScaleInit(scale_x, scale_y);
    Matrix2x3 rotation;
    rotation.RotationInit(radians);
    Matrix2x3 translation;
    translation.TranslateInit(trans_x, trans_y);
    Matrix2x3 tmp;
    tmp.Multiply(&translation, &rotation);
    Multiply(&tmp, &scale);
}

void Matrix2x3::Multiply(const Matrix2x3 *first, const Matrix2x3 *second)
{
#if 0
    a_[0][0] = first->a_[0][0] * second->a_[0][0] + first->a_[0][1] * second->a_[1][0];
    a_[0][1] = first->a_[0][0] * second->a_[0][1] + first->a_[0][1] * second->a_[1][1];
    a_[0][2] = first->a_[0][0] * second->a_[0][2] + first->a_[0][1] * second->a_[1][2] + first->a_[0][2];
    a_[1][0] = first->a_[1][0] * second->a_[1][0] + first->a_[1][1] * second->a_[1][0];
    a_[1][1] = first->a_[1][0] * second->a_[1][1] + first->a_[1][1] * second->a_[1][1];
    a_[1][2] = first->a_[1][0] * second->a_[1][2] + first->a_[1][1] * second->a_[1][2] + first->a_[1][2];
#else
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            a_[j][i] = 0.0f;

            for (int k = 0; k < 2; k++)
            {
                a_[j][i] += first->a_[j][k] * second->a_[k][i];
            }
        }
        a_[j][2] += first->a_[j][2];
    }
#endif
}

float Matrix2x3::SquareSize(void) const
{
#ifdef USE_CROSS_PRODUCT_SIZE
    float cross_product = a_[0][0] * a_[1][1] - a_[1][0] * a_[0][1];
    return fabsf(cross_product);
#else
    float square_length1 = a_[0][0] * a_[0][0] + a_[0][1] * a_[0][1];
    float square_length2 = a_[1][0] * a_[1][0] + a_[1][1] * a_[1][1];
#endif
    return square_length1 > square_length2 ? square_length1 : square_length2;
}

static float frand(float kMin = 0.0f, float kMax = 1.0f)
{
    float f = static_cast<float>(rand()) / RAND_MAX;
    return kMin + f * (kMax - kMin);
}

Fractal::Fractal()
{
    for (int i = 0; i < kNumFunctions; i++)
    {
        function_[i].Set(frand(-1.0f, 1.0f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f));
    }

    // Hard-coded function
    //function_[0].Set(-4 / 128.0f, 83 / 128.0f, -39 / 128.0f, -62 / 128.0f, 38 / 128.0f, -55 / 128.0f);
    //function_[1].Set(69 / 128.0f, -65 / 128.0f, 30 / 128.0f, 50 / 128.0f, 77 / 128.0f, 18 / 128.0f);
    function_[0].Set(-8 / 128.0f, 81 / 128.0f, -110 / 128.0f, 45 / 128.0f, -3 / 128.0f);
    function_[1].Set(20 / 128.0f, 87 / 128.0f, 57 / 128.0f, -47 / 128.0f, 11 / 128.0f);
}

Fractal::~Fractal()
{
}

float Fractal::GetLine(int index, float *start_x, float *start_y, float *end_x, float *end_y)
{
    ImVec2 center = ImVec2(point_[index].a_[0][2], point_[index].a_[1][2]);
    ImVec2 right = ImVec2(point_[index].a_[0][0], point_[index].a_[1][0]);
    ImVec2 down = ImVec2(point_[index].a_[0][1], point_[index].a_[1][1]);

    // Make smaller
    const float kLengthModifier = 1.0f;
    right.x *= kLengthModifier;
    right.y *= kLengthModifier;

    const float kAlphaFactor = 100.0f;
    float width = sqrtf((point_[index].a_[0][1] * point_[index].a_[0][1]) + (point_[index].a_[1][1] * point_[index].a_[1][1]));

    *start_x = center.x - right.x;
    *start_y = center.y - right.y;
    *end_x = center.x + right.x;
    *end_y = center.y + right.y;

    return width;
}

void Fractal::Play(float min_size)
{
    const float kMaxFreq = 20000.0f;
    const float kMinFreq = 40.0f;
    float kMaxLogFreq = logf(kMaxFreq / kMinFreq);

    const int kNumPlayPoints = kMaxNumPoints;
    //const int kNumPlayPoints = 1000;
    Generate(min_size, kNumPlayPoints);

    memset(music, 0, sizeof(music));

    for (int i = 0; i < num_active_points_; i++)
    {
        if (draw_point_[i] &&
            point_[i].a_[0][2] > -1.0f && point_[i].a_[0][2] < 1.0f &&
            point_[i].a_[1][2] > -1.0f && point_[i].a_[1][2] < 1.0f)
        {
#if 0
            // Single point in the middle
            int time_sample = static_cast<int>(MZK_NUMSAMPLES * (0.5f * point_[i].a_[0][2] + 0.5f));
            float log_freq = kMaxLogFreq * (0.5f - 0.5f * point_[i].a_[1][2]);
            float freq = kMinFreq * expf(log_freq);

            //int end_sample = time_sample + 3 * static_cast<int>(MZK_RATE / freq);
            // 50 ms sample
            float sample_duration = MZK_RATE / 1000.0f * 50.0f;
            if (sample_duration < 3 * MZK_RATE / freq) sample_duration = 3 * MZK_RATE / freq;
            int end_sample = time_sample + static_cast<int>(sample_duration);

            for (int sample = time_sample; sample < end_sample; sample++)
            {
                if (sample >= 0 && sample < MZK_NUMSAMPLES)
                {
                    float T = static_cast<float>(sample - time_sample) / sample_duration;
                    float hann = sinf(3.1415f * T);
                    hann *= hann;
                    int input = music[sample];
                    float fT = freq * (sample - time_sample) / MZK_RATE * TWO_PI;
                    input += static_cast<short>(hann * 500.0f * cos(fT));
                    if (input < -32768) input = -32768;
                    if (input > 32767) input = 32767;
                    music[sample] = input;
                }
            }
#else
            // Straight line
            float start[2];
            float end[2];
            float width = GetLine(i, &start[0], &start[1], &end[0], &end[1]);

            int start_sample = static_cast<int>(MZK_NUMSAMPLES * (0.5f * start[0] + 0.5f) - 1);
            int end_sample = static_cast<int>(MZK_NUMSAMPLES * (0.5f * end[0] + 0.5f) + 1);
            int sample_duration = end_sample - start_sample;

            float log_freq = kMaxLogFreq * (0.5f - 0.5f * start[1]);
            float start_freq = kMinFreq * expf(log_freq);
            log_freq = kMaxLogFreq * (0.5f - 0.5f * end[1]);
            float end_freq = kMinFreq * expf(log_freq);
            float freq_step = (end_freq - start_freq) / sample_duration;

            float phase = 0.0f;
            float freq = start_freq;

            for (int sample = start_sample; sample < end_sample; sample++)
            {
                if (sample >= 0 && sample < MZK_NUMSAMPLES)
                {
                    float T = static_cast<float>(sample - start_sample) / sample_duration;
                    float hann = sinf(3.1415f * T);
                    //hann *= hann;
                    int input = music[sample];
                    //float fT = freq * (sample - time_sample) / MZK_RATE * TWO_PI;
                    input += static_cast<short>(hann * 200.0f * cos(phase));
                    if (input < -32768) input = -32768;
                    if (input > 32767) input = 32767;
                    music[sample] = input;
                }
                phase += freq / MZK_RATE * TWO_PI;
                freq += freq_step;
            }
#endif
        }
    }

    memcpy(music, kWavHeader_, sizeof(kWavHeader_));
    sndPlaySound((const char*)&music, SND_ASYNC | SND_MEMORY);

    FILE *fid = fopen("tmp.wav", "wb");
    fwrite(music, 1, sizeof(music), fid);
    fclose(fid);
}

#define IMGUI_WIDTH 640
#define IMGUI_HEIGHT 320
void Fractal::ImGUIDraw(float min_size)
{
    Generate(min_size, kNumDrawPoints);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(IMGUI_WIDTH, IMGUI_HEIGHT), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Preview Window", 0,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    ImVec2 window_size = ImGui::GetWindowSize();

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of overloaded operators, etc.
    // Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your types and ImVec2/ImVec4. 
    // ImGui defines overloaded operators but they are internal to imgui.cpp and not exposed outside (to avoid messing with your types) 
    // In this example we are not using the maths operators! 
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //ImGui::Text("Primitives");
    const ImU32 kColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    for (int i = 0; i < num_active_points_; i++)
    {
        if (draw_point_[i])
        {
            const float kAlphaFactor = 100.0f;
#if 0
            ImVec2 center = ImVec2(window_size.x * 0.5f * point_[i].a_[0][2] + p.x + window_size.x * 0.5f,
                                   window_size.y * 0.5f * point_[i].a_[1][2] + p.y + window_size.y * 0.5f);
            //draw_list->AddRectFilled(center, ImVec2(center.x + 1.0f, center.y + 1.0f), kColor);
            ImVec2 right = ImVec2(window_size.x * 0.5f * point_[i].a_[0][0],
                                  window_size.y * 0.5f * point_[i].a_[1][0]);
            ImVec2 down = ImVec2(window_size.x * 0.5f * point_[i].a_[0][1],
                                 window_size.y * 0.5f * point_[i].a_[1][1]);
            // Make smaller
            const float kLengthModifier = 0.5f;
            right.x *= kLengthModifier;
            right.y *= kLengthModifier;
            
            float width = sqrtf((point_[i].a_[0][1] * point_[i].a_[0][1]) + (point_[i].a_[1][1] * point_[i].a_[1][1]));
            float alpha = kAlphaFactor * width;
            //draw_list->AddQuadFilled(ImVec2(center.x + right.x + down.x, center.y + right.y + down.y),
            //                         ImVec2(center.x - right.x + down.x, center.y - right.y + down.y),
            //                         ImVec2(center.x - right.x - down.x, center.y - right.y - down.y),
            //                         ImVec2(center.x + right.x - down.x, center.y + right.y - down.y), kColor);
            draw_list->AddLine(ImVec2(center.x + right.x, center.y + right.y),
                               ImVec2(center.x - right.x, center.y - right.y),
                               ImColor(1.0f, 1.0f, 1.0f, alpha));
#else
            ImVec2 start;
            ImVec2 end;
            float width = GetLine(i, &start.x, &start.y, &end.x, &end.y);
            draw_list->AddLine(ImVec2(window_size.x * 0.5f * start.x + p.x + window_size.x * 0.5f,
                                      window_size.y * 0.5f * start.y + p.y + window_size.y * 0.5f),
                               ImVec2(window_size.x * 0.5f * end.x + p.x + window_size.x * 0.5f,
                                      window_size.y * 0.5f * end.y + p.y + window_size.y * 0.5f),
                               ImColor(1.0f, 1.0f, 1.0f, width * kAlphaFactor));
#endif
        }
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
}

void Fractal::ImGUIControl(void)
{
    ImGui::SetNextWindowSize(ImVec2(700, 320));
    if (!ImGui::Begin("Fractal Control", 0))
    {
        ImGui::End();
        return;
    }

    bool first_index = true;
    for (int function_id = 0; function_id < kNumFunctions; function_id++)
    {
        //ImGui::Text("Function %d", function_id + 1);
        for (int i = 0; i < 5; i++)
        {
            char name[128];
            sprintf_s<128>(name, "f%d p%d", function_id + 1, i + 1);
            //ImGui::SliderFloat(name, &function_[function_id].a_[i][j], -1.0f, 1.0f);

            if (!first_index) ImGui::SameLine();
            first_index = false;

            int index = i;
            ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(index / 7.0f, 0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(index / 7.0f, 0.6f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(index / 7.0f, 0.7f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(index / 7.0f, 0.9f, 0.9f));
            ImGui::PushID(name);
            //ImGui::VSliderFloat("##", ImVec2(18, 280), &function_[function_id].a_[i][j], -1.0f, 1.0f, "");
            int int_slider_pos = static_cast<int>(function_[function_id].parameters[index] * 128);
            ImGui::VSliderInt("##", ImVec2(30, 256), &int_slider_pos, -128, 127);
            function_[function_id].parameters[index] = static_cast<float>(int_slider_pos) / 128.0f;
            ImGui::PopID();
            ImGui::PopStyleColor(4);
        }
        function_[function_id].CreateMatrix();
    }

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(300, 150));
    if (!ImGui::Begin("Fractal Final Transform", 0))
    {
        ImGui::End();
        return;
    }

    ImGui::SliderFloat("Rotation", &final_rotation_, 0.0f, 1.0f);
    ImGui::SliderFloat("Scale X", &(final_scale_[0]), -2.0f, 2.0f);
    ImGui::SliderFloat("Scale Y", &(final_scale_[1]), -2.0f, 2.0f);
    ImGui::SliderFloat("Translate X", &(final_translation_[0]), -1.0f, 1.0f);
    ImGui::SliderFloat("Translate Y", &(final_translation_[1]), -1.0f, 1.0f);
    ImGui::End();
}

void Fractal::Generate(float min_size, int max_num_points)
{
    if (max_num_points > kMaxNumPoints) max_num_points = kMaxNumPoints;

    point_[0].Set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    draw_point_[0] = true;
    num_active_points_ = 1;
    float min_square_size = min_size * min_size;

    for (int read_point_index = 0; read_point_index < num_active_points_; read_point_index++)
    {
        // Pre-emptive quit if nothing more can be done
        if (num_active_points_ == max_num_points) break;

        // Check for size [TODO: Implement the size check (with parameter?)]
        if (point_[read_point_index].SquareSize() > min_square_size)
        {
            for (int function_id = 0; function_id < kNumFunctions; function_id++)
            {
                // Pre-emptive quit if nothing more can be done
                if (num_active_points_ == max_num_points) break;

                point_[num_active_points_].Multiply(&(point_[read_point_index]), &(function_[function_id].matrix));
                draw_point_[num_active_points_] = true;
                num_active_points_++;

                // The point is getting split. So no need to draw it.
                draw_point_[read_point_index] = false;
            }
        }
    }

    Matrix2x3 final_transform;
    final_transform.a_[0][0] = cosf(final_rotation_ * 3.1415f * 2.0f) * final_scale_[0];
    final_transform.a_[0][1] = -sinf(final_rotation_ * 3.1415f * 2.0f) * final_scale_[0];
    final_transform.a_[1][0] = sinf(final_rotation_ * 3.1415f * 2.0f) * final_scale_[1];
    final_transform.a_[1][1] = cosf(final_rotation_ * 3.1415f * 2.0f) * final_scale_[1];
    final_transform.a_[0][2] = final_translation_[0];
    final_transform.a_[1][2] = final_translation_[1];

    for (int i = 0; i < num_active_points_; i++)
    {
        if (draw_point_[i])
        {
            Matrix2x3 transformed;
            transformed.Multiply(&final_transform, &(point_[i]));
            point_[i] = transformed;
        }
    }
}
