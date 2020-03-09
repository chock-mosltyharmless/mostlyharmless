#include "ProjectionRoom.h"

#include "../imgui/imgui.h"

ProjectionRoom::ProjectionRoom()
{
    // Front 4, back 4, counterclockwise starting top left
    edge_[0].coord[0] = 0.25f; edge_[0].coord[1] = 0.25f; edge_[0].coord[2] = 2.0f;
    edge_[1].coord[0] = -0.25f; edge_[1].coord[1] = 0.25f; edge_[1].coord[2] = 2.0f;
    edge_[2].coord[0] = -0.25f; edge_[2].coord[1] = -0.25f; edge_[2].coord[2] = 2.0f;
    edge_[3].coord[0] = 0.25f; edge_[3].coord[1] = -0.25f; edge_[3].coord[2] = 2.0f;

    edge_[4].coord[0] = 1.0f; edge_[4].coord[1] = 1.0f; edge_[4].coord[2] = 0.5f;
    edge_[5].coord[0] = -1.0f; edge_[5].coord[1] = 1.0f; edge_[5].coord[2] = 0.5f;
    edge_[6].coord[0] = -1.0f; edge_[6].coord[1] = -1.0f; edge_[6].coord[2] = 0.5f;
    edge_[7].coord[0] = 1.0f; edge_[7].coord[1] = -1.0f; edge_[7].coord[2] = 0.5f;
}

ProjectionRoom::~ProjectionRoom()
{
}

void ProjectionRoom::GetNormal(const float *a, const float *b, const float *c, float *normal)
{
    float right[3];
    float up[3];
    for (int dim = 0; dim < 3; dim++)
    {
        right[dim] = c[dim] - a[dim];
        up[dim] = b[dim] - a[dim];
    }

    normal[0] = right[1] * up[2] - right[2] * up[1];
    normal[1] = right[2] * up[0] - right[0] * up[2];
    normal[2] = right[0] * up[1] - right[1] * up[0];

    float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    for (int dim = 0; dim < 3; dim++)
    {
        normal[dim] /= length;
    }
}

void ProjectionRoom::RenderAll(void)
{
    float normal[3];

    for (int i = 0; i < 8; i++)
    {
        edge_[i].Update();
    }

    glBegin(GL_QUADS);
    // center:
    GetNormal(edge_[0].location, edge_[1].location, edge_[2].location, normal);
    glNormal3fv(normal);
    glVertex3fv(edge_[0].gl_coord);
    glVertex3fv(edge_[1].gl_coord);
    glVertex3fv(edge_[2].gl_coord);
    glVertex3fv(edge_[3].gl_coord);

    // right:
    GetNormal(edge_[0].location, edge_[3].location, edge_[7].location, normal);
    glNormal3fv(normal);
    glVertex3fv(edge_[0].gl_coord);
    glVertex3fv(edge_[3].gl_coord);
    glVertex3fv(edge_[7].gl_coord);
    glVertex3fv(edge_[4].gl_coord);

    // left:
    GetNormal(edge_[1].location, edge_[5].location, edge_[6].location, normal);
    glNormal3fv(normal);
    glVertex3fv(edge_[1].gl_coord);
    glVertex3fv(edge_[5].gl_coord);
    glVertex3fv(edge_[6].gl_coord);
    glVertex3fv(edge_[2].gl_coord);

    // top:
    GetNormal(edge_[4].location, edge_[5].location, edge_[1].location, normal);
    glNormal3fv(normal);
    glVertex3fv(edge_[4].gl_coord);
    glVertex3fv(edge_[5].gl_coord);
    glVertex3fv(edge_[1].gl_coord);
    glVertex3fv(edge_[0].gl_coord);

    // bottom:
    GetNormal(edge_[3].location, edge_[2].location, edge_[6].location, normal);
    glNormal3fv(normal);
    glVertex3fv(edge_[3].gl_coord);
    glVertex3fv(edge_[2].gl_coord);
    glVertex3fv(edge_[6].gl_coord);
    glVertex3fv(edge_[7].gl_coord);

    glEnd();
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
            float min_value = -1.0f;
            float max_value = 1.0f;
            if (dim == 2)
            {
                min_value = 0.1f;
                max_value = 5.0f;
            }
            ImGui::VSliderFloat("##", ImVec2(40, 270), &edge_[i].coord[dim], min_value, max_value);
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
