#include "ProjectionRoom.h"

#include "../imgui/imgui.h"

ProjectionRoom::ProjectionRoom()
{
}

ProjectionRoom::~ProjectionRoom()
{
}

void ProjectionRoom::ImGUIControl(void)
{
    ImGui::SetNextWindowSize(ImVec2(1170, 320));
    if (!ImGui::Begin("Room Control", 0))
    {
        ImGui::End();
        return;
    }

    // Edge positions
    // I need to change this to be in-graphics...
    bool first_index = true;
    for (int i = 0; i < 8; i++)
    {
        for (int dim = 0; dim < 3; dim++)
        {
            char name[128];
            sprintf_s<128>(name, "e%d c%d", i + 1, dim + 1);
            //ImGui::SliderFloat(name, &function_[function_id].a_[i][j], -1.0f, 1.0f);

            if (!first_index) ImGui::SameLine();
            first_index = false;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(dim / 3.0f, 0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(dim / 3.0f, 0.6f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(dim / 3.0f, 0.7f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(dim / 3.0f, 0.9f, 0.9f));
            ImGui::PushID(name);
            //ImGui::VSliderFloat("##", ImVec2(18, 280), &function_[function_id].a_[i][j], -1.0f, 1.0f, "");
            ImGui::VSliderFloat("##", ImVec2(40, 270), &edge_[i].coord[dim], -3.0f, 3.0f);
            ImGui::PopID();
            ImGui::PopStyleColor(4);
        }
    }

    ImGui::End();

#if 0
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
#endif
}
