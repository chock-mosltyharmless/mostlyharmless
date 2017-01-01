#include "stdafx.h"
#include "Line.h"

const float Line::kLineWidth = 0.0025f;
const float Line::kMinLineWidth = 0.002f;
const float Line::kNeighborInterpolationDistance = 0.1f;

Line::Line()
{
}


Line::~Line()
{
}

void Line::AddNode(float x, float y, bool make_fancy) {
    nodes_.push_back(std::make_pair(x, y));
    if (make_fancy) MakeFancy();
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

int Line::Export(FILE *file) {
    MakeFancy();
    fwrite(&kExportMagicNumber, sizeof(kExportMagicNumber), 1, file);
    int num_elements = fancy_nodes_.size();
    fwrite(&num_elements, sizeof(num_elements), 1, file);
    for (int i = 0; i < num_elements; i++) {
        fwrite(&(fancy_nodes_[i].first), sizeof(float), 1, file);
        fwrite(&(fancy_nodes_[i].second), sizeof(float), 1, file);
    }

    return 0;
}

void Line::MakeFancy(void) {
    fancy_nodes_.clear();

    for (int i = 0; i < (int)nodes_.size(); i++) {
        fancy_nodes_.push_back(nodes_[i]);
    }
    for (int i = 0; i < (int)nodes_.size(); i++) {
        // Check how much weight I get total
        float weight = 0.0f;
        
        // Integrate weights to the left (including center)
        float distance_so_far = 0.0f;
        for (int j = i; j >= 0; j--) {
            float dx = nodes_[i].first - nodes_[j].first;
            float dy = nodes_[i].second - nodes_[j].second;
            distance_so_far += sqrtf(dx * dx + dy * dy);
            float amount = kNeighborInterpolationDistance - distance_so_far;
            if (amount < 0.0f) {
                amount = 0.0f;
                break;
            }
            weight += amount;
        }

        // Integrate weights to the right
        distance_so_far = 0.0f;
        for (int j = i + 1; j < (int)nodes_.size(); j++) {
            float dx = nodes_[i].first - nodes_[j].first;
            float dy = nodes_[i].second - nodes_[j].second;
            distance_so_far += sqrtf(dx * dx + dy * dy);
            float amount = kNeighborInterpolationDistance - distance_so_far;
            if (amount < 0.0f) {
                amount = 0.0f;
                break;
            }
            weight += amount;
        }

        // Interpolate
        fancy_nodes_[i].first = 0;
        fancy_nodes_[i].second = 0;

        // Interpolate to the left (including center)
        distance_so_far = 0.0f;
        for (int j = i; j >= 0; j--) {
            float dx = nodes_[i].first - nodes_[j].first;
            float dy = nodes_[i].second - nodes_[j].second;
            distance_so_far += sqrtf(dx * dx + dy * dy);
            float amount = kNeighborInterpolationDistance - distance_so_far;
            if (amount < 0.0f) {
                amount = 0.0f;
                break;
            }
            fancy_nodes_[i].first += amount * nodes_[j].first / weight;
            fancy_nodes_[i].second += amount * nodes_[j].second / weight;
        }

        // Interpolate to the right
        distance_so_far = 0.0f;
        for (int j = i + 1; j < (int)nodes_.size(); j++) {
            float dx = nodes_[i].first - nodes_[j].first;
            float dy = nodes_[i].second - nodes_[j].second;
            distance_so_far += sqrtf(dx * dx + dy * dy);
            float amount = kNeighborInterpolationDistance - distance_so_far;
            if (amount < 0.0f) {
                amount = 0.0f;
                break;
            }
            fancy_nodes_[i].first += amount * nodes_[j].first / weight;
            fancy_nodes_[i].second += amount * nodes_[j].second / weight;
        }
    }
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
        AddNode(pos[0], pos[1], false);
    }

    // Create fancy nodes by interpolating borders
    MakeFancy();

    return 0;
}

void Line::DrawFancy(void) {
    if (fancy_nodes_.size() < 2) return;

    // Get the length of the line
    float length = 0.0f;
    for (int i = 0; i < (int)fancy_nodes_.size() - 1; i++) {
        float dx = fancy_nodes_[i + 1].first - fancy_nodes_[i].first;
        float dy = fancy_nodes_[i + 1].second - fancy_nodes_[i].second;
        length += sqrtf(dx * dx + dy * dy);
    }
    if (length < 0.001f) length = 0.001f;
    float width = kLineWidth * 2.0f;

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Start is just a point
    int index = 0;
    glTexCoord2f(0.0f, 1.0f);  // top
    glVertex3f(fancy_nodes_[index].first, fancy_nodes_[index].second, 0.9f);
    glTexCoord2f(0.0f, 1.0f);  // bottom
    glVertex3f(fancy_nodes_[index].first, fancy_nodes_[index].second, 0.9f);

    // Draw middle lines
    float current_length = 0.0f;
    for (int i = 1; i < (int)fancy_nodes_.size() - 1; i++) {
        float dif_x = fancy_nodes_[i + 1].first - fancy_nodes_[i - 1].first;
        float dif_y = fancy_nodes_[i + 1].second - fancy_nodes_[i - 1].second;
        float dx = fancy_nodes_[i].first - fancy_nodes_[i - 1].first;
        float dy = fancy_nodes_[i].second - fancy_nodes_[i - 1].second;
        current_length += sqrtf(dx * dx + dy * dy);
        //float t = 2.0f * current_length / length;
        //if (t > 1.0f) t = 2.0f - t;
        float t = 1.0f - current_length / length;
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
        float up_x = fancy_nodes_[i].first - normal_x;
        float up_y = fancy_nodes_[i].second - normal_y;
        float down_x = fancy_nodes_[i].first + normal_x;
        float down_y = fancy_nodes_[i].second + normal_y;

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
    index = fancy_nodes_.size() - 1;
    glTexCoord2f(1.0f, 1.0f);  // bottom
    glVertex3f(fancy_nodes_[index].first, fancy_nodes_[index].second, 0.9f);
    glTexCoord2f(1.0f, 1.0f);  // top
    glVertex3f(fancy_nodes_[index].first, fancy_nodes_[index].second, 0.9f);

    glEnd();
}

void Line::Draw(float alpha) {
    if (nodes_.size() < 2) return;
    
    glBegin(GL_QUADS);
    float color_alpha = (alpha - 0.3f) / 0.7f;
    glColor4f(color_alpha * color_alpha, color_alpha, 1.0f, alpha);

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
