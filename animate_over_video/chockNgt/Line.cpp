#include "stdafx.h"
#include "Line.h"

const float Line::kLineWidth = 0.006f;
const float Line::kMinLineWidth = 0.001f;

Line::Line()
{
}


Line::~Line()
{
}

void Line::AddNode(float x, float y) {
    nodes_.push_back(std::make_pair(x, y));
}

int Line::Save(FILE * file) {
    fwrite(&kMagicNumber, sizeof(kMagicNumber), 1, file);
    int num_elements = nodes_.size();
    fwrite(&num_elements, sizeof(num_elements), 1, file);
    for (int i = 0; i < num_elements; i++) {
        fwrite(&(nodes_[i].first), sizeof(float), 1, file);
        fwrite(&(nodes_[i].second), sizeof(float), 1, file);
    }

    return 0;
}

int Line::Load(FILE * file) {
    unsigned int ref_val;
    fread(&ref_val, sizeof(ref_val), 1, file);
    if (ref_val != kMagicNumber) {
        // Not the correct thing.
        return -1;
    }

    int num_elements;
    fread(&num_elements, sizeof(num_elements), 1, file);
    
    for (int i = 0; i < num_elements; i++) {
        float pos[2];
        fread(pos, sizeof(float), 2, file);
        AddNode(pos[0], pos[1]);
    }

    return 0;
}

void Line::DrawFancy(void) {
    if (nodes_.size() < 2) return;

    // Get the length of the line
    float length = 0.0f;
    for (int i = 0; i < (int)nodes_.size() - 1; i++) {
        float dx = nodes_[i + 1].first - nodes_[i].first;
        float dy = nodes_[i + 1].second - nodes_[i].second;
        length += sqrtf(dx * dx + dy * dy);
    }
    if (length < 0.001f) length = 0.001f;
    float width = kLineWidth * 2.0f;

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Start is just a point
    int index = 0;
    glTexCoord2f(0.0f, 1.0f);  // top
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);
    glTexCoord2f(0.0f, 1.0f);  // bottom
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);

    // Draw middle lines
    float current_length = 0.0f;
    for (int i = 1; i < (int)nodes_.size() - 1; i++) {
        float dif_x = nodes_[i + 1].first - nodes_[i - 1].first;
        float dif_y = nodes_[i + 1].second - nodes_[i - 1].second;
        float dx = nodes_[i].first - nodes_[i - 1].first;
        float dy = nodes_[i].second - nodes_[i - 1].second;
        current_length += sqrtf(dx * dx + dy * dy);
        //float t = 2.0f * current_length / length;
        //if (t > 1.0f) t = 2.0f - t;
        float t = current_length / length;
        if (t > 1.0f) t = 1.0f;
        t = 0.5f * t + 0.5f * t * t;
        t = t * t;
        t = sin(t * 3.1415f);
        float current_width = width * t;
        if (current_width < kMinLineWidth) current_width = kMinLineWidth;
        float normal_x = -dif_y;
        float normal_y = dif_x;
        float inv_normal_length = 1.0f / sqrtf(normal_x * normal_x + normal_y * normal_y);
        normal_x *= inv_normal_length * current_width;
        normal_y *= inv_normal_length * current_width;
        float up_x = nodes_[i].first - normal_x;
        float up_y = nodes_[i].second - normal_y;
        float down_x = nodes_[i].first + normal_x;
        float down_y = nodes_[i].second + normal_y;

        glTexCoord2f(0.5f, 0.0f);  // bottom
        glVertex3f(down_x, down_y, 0.9f);
        glTexCoord2f(0.5f, 1.0f);  // top
        glVertex3f(up_x, up_y, 0.9f);

        glTexCoord2f(0.5f, 1.0f);  // top
        glVertex3f(up_x, up_y, 0.9f);
        glTexCoord2f(0.5f, 0.0f);  // bottom
        glVertex3f(down_x, down_y, 0.9f);
    }

    // End is just a point
    index = nodes_.size() - 1;
    glTexCoord2f(1.0f, 1.0f);  // bottom
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);
    glTexCoord2f(1.0f, 1.0f);  // top
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);

    glEnd();
}

void Line::Draw(float alpha) {
    if (nodes_.size() < 2) return;
    
    glBegin(GL_QUADS);
    glColor4f(alpha * alpha, alpha, 1.0f, alpha);

    // Start is just a point
    int index = 0;
    glTexCoord2f(0.0f, 1.0f);  // top
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);
    glTexCoord2f(0.0f, 1.0f);  // bottom
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);

    // Draw middle lines
    for (int i = 1; i < (int)nodes_.size() - 1; i++) {
        float dif_x = nodes_[i + 1].first - nodes_[i - 1].first;
        float dif_y = nodes_[i + 1].second - nodes_[i - 1].second;
        float normal_x = -dif_y;
        float normal_y = dif_x;
        float inv_normal_length = 1.0f / sqrtf(normal_x * normal_x + normal_y * normal_y);
        normal_x *= inv_normal_length * kLineWidth;
        normal_y *= inv_normal_length * kLineWidth;
        float up_x = nodes_[i].first - normal_x;
        float up_y = nodes_[i].second - normal_y;
        float down_x = nodes_[i].first + normal_x;
        float down_y = nodes_[i].second + normal_y;

        glTexCoord2f(0.5f, 0.0f);  // bottom
        glVertex3f(down_x, down_y, 0.9f);
        glTexCoord2f(0.5f, 1.0f);  // top
        glVertex3f(up_x, up_y, 0.9f);

        glTexCoord2f(0.5f, 1.0f);  // top
        glVertex3f(up_x, up_y, 0.9f);
        glTexCoord2f(0.5f, 0.0f);  // bottom
        glVertex3f(down_x, down_y, 0.9f);
    }

    // End is just a point
    index = nodes_.size() - 1;
    glTexCoord2f(1.0f, 1.0f);  // bottom
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);
    glTexCoord2f(1.0f, 1.0f);  // top
    glVertex3f(nodes_[index].first, nodes_[index].second, 0.9f);

    glEnd();
}
