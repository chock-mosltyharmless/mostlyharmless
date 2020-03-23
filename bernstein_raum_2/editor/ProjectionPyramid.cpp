#include "ProjectionPyramid.h"

#include "../imgui/imgui.h"

ProjectionPyramid::ProjectionPyramid()
{
    // Center
    edge_[0].coord[0] = 0.0f; edge_[0].coord[1] = 0.0f; edge_[0].coord[2] = 3.0f;
    // Top-Right
    edge_[1].coord[0] = 2.0f; edge_[1].coord[1] = 1.0f; edge_[1].coord[2] = 1.0f;
    // Top-Left
    edge_[2].coord[0] = -2.0f; edge_[2].coord[1] = 1.0f; edge_[2].coord[2] = 1.0f;
    // Bottom
    edge_[3].coord[0] = 0.0f; edge_[3].coord[1] = -3.0f; edge_[3].coord[2] = 1.0f;

    FILE *fid = fopen("textures/calibration.bin", "rb");
    if (fid)
    {
        for (int i = 0; i < 4; i++)
        {
            fread(edge_[i].coord, sizeof(float), 3, fid);
        }
    }
}

ProjectionPyramid::~ProjectionPyramid()
{
    FILE *fid = fopen("textures/calibration.bin", "wb");
    if (fid)
    {
        for (int i = 0; i < 4; i++)
        {
            fwrite(edge_[i].coord, sizeof(float), 3, fid);
        }
    }
}

void ProjectionPyramid::GetNormalAngles(float angles[3])
{
    float normal[3][3];  // For the three walls
    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3;
        GetNormal(edge_[0].location, edge_[i + 1].location, edge_[j + 1].location, normal[i]);
    }

    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3;
        float scalar_product = 0.0f;
        for (int dim = 0; dim < 3; dim++)
        {
            scalar_product += normal[i][dim] * normal[j][dim];
        }
        angles[i] = (float)acos(scalar_product) * 180.0f / 3.1415f;
    }
}

void ProjectionPyramid::GetNormal(const float *a, const float *b, const float *c, float *normal)
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

void ProjectionPyramid::RenderAll(void)
{
    float normal[3];

    for (int i = 0; i < 4; i++)
    {
        edge_[i].Update();
    }

    // Calculate the length of the lines in real space
    float lengths[3];
    float max_length = 1.0e-5;
    for (int i = 0; i < 3; i++)
    {
        lengths[i] = 0.0f;
        for (int dim = 0; dim < 3; dim++)
        {
            float difference = edge_[0].location[dim] - edge_[i + 1].location[dim];
            lengths[i] += (difference * difference);
        }
        lengths[i] = sqrtf(lengths[i]);
        if (lengths[i] > max_length) max_length = lengths[i];
    }
    for (int i = 0; i < 3; i++)
    {
        lengths[i] /= max_length;
    }

    glBegin(GL_TRIANGLES);
    // top:
    GetNormal(edge_[0].location, edge_[1].location, edge_[2].location, normal);
    glNormal3fv(normal);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(edge_[0].gl_coord);
    glTexCoord2f(lengths[0], 0.0f);
    glVertex3fv(edge_[1].gl_coord);
    glTexCoord2f(0.0f, lengths[1]);
    glVertex3fv(edge_[2].gl_coord);

    // left:
    GetNormal(edge_[0].location, edge_[2].location, edge_[3].location, normal);
    glNormal3fv(normal);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(edge_[0].gl_coord);
    glTexCoord2f(lengths[1], 0.0f);
    glVertex3fv(edge_[2].gl_coord);
    glTexCoord2f(0.0f, lengths[2]);
    glVertex3fv(edge_[3].gl_coord);

    // right:
    GetNormal(edge_[0].location, edge_[3].location, edge_[1].location, normal);
    glNormal3fv(normal);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(edge_[0].gl_coord);
    glTexCoord2f(0.0f, lengths[2]);
    glVertex3fv(edge_[3].gl_coord);
    glTexCoord2f(lengths[0], 0.0f);
    glVertex3fv(edge_[1].gl_coord);

    glEnd();
}

void ProjectionPyramid::ImGUIControl(void)
{
    ImGui::SetNextWindowSize(ImVec2(590, 320));
    if (!ImGui::Begin("Room Control", 0))
    {
        ImGui::End();
        return;
    }

    // Edge positions
    // I need to change this to be in-graphics...
    bool first_index = true;
    for (int i = 0; i < 4; i++)
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
            float min_value = -3.0f;
            float max_value = 3.0f;
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
}
