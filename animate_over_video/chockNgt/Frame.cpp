#include "stdafx.h"
#include "Frame.h"
#include "Configuration.h"

Frame::Frame() {
}


Frame::~Frame() {
}

void Frame::StartNewLine() {
    // TODO: Is this really what I want to do? It looks like a mess.
    // Damn you C++. I really hate you.
    lines_.push_back(Line());
}

void Frame::AddLineNode(float x, float y, bool make_fancy) {
    int current_line = lines_.size() - 1;
    if (current_line < 0) return;  // Tried to add a node before calling StartNewLine()

    lines_[current_line].AddNode(x, y, make_fancy);
}

void Frame::DeleteLastLine() {
    if (lines_.size() > 0) lines_.pop_back();
}

int Frame::Save(FILE *file, char *error_string) {
    fwrite(&kMagicNumber, sizeof(kMagicNumber), 1, file);
    int num_elements = lines_.size();
    fwrite(&num_elements, sizeof(num_elements), 1, file);

    for (int i = 0; i < num_elements; i++) {
        lines_[i].Save(file);
    }

    return 0;
}

int Frame::Load(FILE *file, char *error_string) {
    unsigned int ref_val;
    fread(&ref_val, sizeof(ref_val), 1, file);
    if (ref_val != kMagicNumber) {
        // Not the correct thing.
        snprintf(error_string, MAX_ERROR_LENGTH, "File does not contain a frame");
        return -1;
    }

    int num_elements;
    fread(&num_elements, sizeof(num_elements), 1, file);

    lines_.clear();
    for (int i = 0; i < num_elements; i++) {
        lines_.push_back(Line());
        lines_[i].Load(file);
    }

    return 0;
}

int Frame::Draw(TextureManager *texture_manager, char *error_string, float alpha) {
    GLuint tex_id;
    int ret_val = texture_manager->getTextureID("line.png", &tex_id, error_string);
    if (ret_val < 0) return ret_val;
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int num_lines = lines_.size();
    for (int i = 0; i < num_lines; i++) {
        lines_[i].Draw(alpha);
    }

    glDisable(GL_BLEND);

    return 0;
}

int Frame::DrawFancy(TextureManager *texture_manager, char *error_string) {
    GLuint tex_id;
    int ret_val = texture_manager->getTextureID("white.png", &tex_id, error_string);
    if (ret_val < 0) return ret_val;
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int num_lines = lines_.size();
    for (int i = 0; i < num_lines; i++) {
        lines_[i].DrawFancy();
    }

    glDisable(GL_BLEND);

    return 0;
}