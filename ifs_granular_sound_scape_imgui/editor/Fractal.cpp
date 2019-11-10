#include "Fractal.h"

#pragma comment(lib,"winmm.lib")

#include "../imgui/imgui.h"

#include <Windows.h>
#include <math.h>
#include <memory.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>

#define MZK_RATE 44100
#define MZK_NUMCHANNELS 1
// 5 second audio
#define MZK_NUMSAMPLES (5 * MZK_RATE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES * MZK_NUMCHANNELS)

static const int kWavHeader[11] = {
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
        function_[i].Set(frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f), frand(-0.8f, 0.8f));
    }
}

Fractal::~Fractal()
{
}

#define IMGUI_WIDTH 640
#define IMGUI_HEIGHT 320
void Fractal::ImGUIDraw(float min_size)
{
    Generate(min_size, 10000);

    ImGui::SetNextWindowSize(ImVec2(IMGUI_WIDTH, IMGUI_HEIGHT), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Preview Window", 0,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of overloaded operators, etc.
    // Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your types and ImVec2/ImVec4. 
    // ImGui defines overloaded operators but they are internal to imgui.cpp and not exposed outside (to avoid messing with your types) 
    // In this example we are not using the maths operators! 
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //ImGui::Text("Primitives");
    const ImU32 kColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    
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

            ImVec2 center = ImVec2(IMGUI_WIDTH * 0.5f * transformed.a_[0][2] + p.x + IMGUI_WIDTH * 0.5f,
                                   IMGUI_HEIGHT * 0.5f * transformed.a_[1][2] + p.y + IMGUI_HEIGHT * 0.5f);
            //draw_list->AddRectFilled(center, ImVec2(center.x + 1.0f, center.y + 1.0f), kColor);
            ImVec2 right = ImVec2(IMGUI_WIDTH * 0.5f * transformed.a_[0][0],
                                  IMGUI_HEIGHT * 0.5f * transformed.a_[1][0]);
            ImVec2 down = ImVec2(IMGUI_WIDTH * 0.5f * transformed.a_[0][1],
                                 IMGUI_HEIGHT * 0.5f * transformed.a_[1][1]);
            // Make smaller
            const float size = 0.5f;
            right.x *= size;
            right.y *= size;
            down.x *= size;
            down.y *= size;
            draw_list->AddQuadFilled(ImVec2(center.x + right.x + down.x, center.y + right.y + down.y),
                                     ImVec2(center.x - right.x + down.x, center.y - right.y + down.y),
                                     ImVec2(center.x - right.x - down.x, center.y - right.y - down.y),
                                     ImVec2(center.x + right.x - down.x, center.y + right.y - down.y), kColor);
        }
    }
    
    
    ImGui::End();
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
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                char name[128];
                sprintf_s<128>(name, "f%d a%d%d", function_id + 1, i + 1, j + 1);
                //ImGui::SliderFloat(name, &function_[function_id].a_[i][j], -1.0f, 1.0f);

                if (!first_index) ImGui::SameLine();
                first_index = false;

                int index = i * 3 + j;
                ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(index / 7.0f, 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(index / 7.0f, 0.6f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(index / 7.0f, 0.7f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(index / 7.0f, 0.9f, 0.9f));
                ImGui::PushID(name);
                //ImGui::VSliderFloat("##", ImVec2(18, 280), &function_[function_id].a_[i][j], -1.0f, 1.0f, "");
                int int_slider_pos = static_cast<int>(function_[function_id].a_[i][j] * 128);
                ImGui::VSliderInt("##", ImVec2(30, 256), &int_slider_pos, -128, 127);
                function_[function_id].a_[i][j] = static_cast<float>(int_slider_pos) / 128.0f;
                ImGui::PopID();
                ImGui::PopStyleColor(4);
            }
        }
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

                point_[num_active_points_].Multiply(&(point_[read_point_index]), &(function_[function_id]));
                draw_point_[num_active_points_] = true;
                num_active_points_++;

                // The point is getting split. So no need to draw it.
                draw_point_[read_point_index] = false;
            }
        }
    }
}

void Fractal::Play(void)
{
    short *music = new short[MZK_NUMSAMPLESC + sizeof(kWavHeader)];
    memset(music, 0, MZK_NUMSAMPLESC * sizeof(short) + sizeof(kWavHeader));

    memcpy(music, kWavHeader, 44);
    sndPlaySound((const char*)&music, SND_ASYNC | SND_MEMORY);

    delete [] music;
}
