#pragma once

#include "TextureManager.h"

#define SIGN_TEXTURE "sign.png"
#define STRINGS_TEXTURE "strings.png"

class Sign
{
public:
    Sign();
    virtual ~Sign();

    // time is relative to when to start, negative time is no drawing
    static void Draw(float time, TextureManager *texture_manager);
};

