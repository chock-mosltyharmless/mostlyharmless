#include "stdafx.h"
#include "Sign.h"

#include "Configuration.h"
#include "chockNgt.h"

Sign::Sign() {
}


Sign::~Sign() {
}

void Sign::Draw(float time, TextureManager * texture_manager) {
    GLuint tex_id;
    char error_string[MAX_ERROR_LENGTH + 1];

    if (time < 0) return;
    
    texture_manager->getTextureID(SIGN_TEXTURE, &tex_id, error_string);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    DrawQuad(-0.5683f, 0.5783f, 0.9578f, 0.7556f, 1.0f);
}
