#pragma once

#include "TextureManager.h"

#define SIGN_TEXTURE "sign.png"
#define STRINGS_TEXTURE "strings.png"

class Sign
{
public:
    Sign();
    virtual ~Sign();

    // time is relative to when to start, negative time is for zoom-out
    static void Draw(float time, TextureManager *texture_manager,
                     float *text_start_x, float *text_start_y, float *text_width);
};

