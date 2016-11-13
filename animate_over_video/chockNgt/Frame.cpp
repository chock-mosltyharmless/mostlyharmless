#include "stdafx.h"
#include "Frame.h"


Frame::Frame()
{
}


Frame::~Frame()
{
}

void Frame::StartNewLine() {
    // TODO: Is this really what I want to do? It looks like a mess.
    // Damn you C++. I really hate you.
    lines_.push_back(Line());
}

void Frame::AddLineNode(float x, float y) {
    int current_line = lines_.size() - 1;
    if (current_line < 0) return;  // Tried to add a node before calling StartNewLine()

    lines_[current_line].AddNode(x, y);
}

void Frame::DeleteLastLine() {
    if (lines_.size() > 0) lines_.pop_back();
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