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
    lines_.pop_back();
}

int Frame::Draw(TextureManager *texture_manager, char *error_string) {
    GLuint tex_id;
    int ret_val = texture_manager->getTextureID("line.png", &tex_id, error_string);
    if (ret_val < 0) return ret_val;
    glBindTexture(GL_TEXTURE_2D, tex_id);
    
    int num_lines = lines_.size();
    for (int i = 0; i < num_lines; i++) {
        lines_[i].Draw();
    }

    return 0;
}