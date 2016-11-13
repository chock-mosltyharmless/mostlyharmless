#pragma once

#include <vector>

#include "Line.h"
#include "TextureManager.h"

class Frame
{
public:
    Frame();
    virtual ~Frame();

    // Inidicate that there will be one more line
    void StartNewLine();
    // Add one more line to the most recently created node
    void AddLineNode(float x, float y);
    // Delete the last line that was created by calling StartNewLine()
    void DeleteLastLine();

    int Save(FILE *file, char *error_string);
    int Load(FILE *file, char *error_string);

    int Draw(TextureManager *texture_manager, char *error_string, float alpha = 1.0f);

private:
    static const unsigned int kMagicNumber = 0x8870bc43;

    std::vector<Line> lines_;
};

