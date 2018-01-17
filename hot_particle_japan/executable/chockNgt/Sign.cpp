#include "stdafx.h"
#include "Sign.h"

#include "Configuration.h"
#include "chockNgt.h"

Sign::Sign() {
}


Sign::~Sign() {
}

void Sign::Draw(float time, TextureManager * texture_manager,
                float *text_start_x, float *text_start_y, float *text_width) {
    GLuint tex_id;
    char error_string[MAX_ERROR_LENGTH + 1];

    if (time < 0) return;
    
    texture_manager->getTextureID(SIGN_TEXTURE, &tex_id, error_string);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    float xpos = -0.5f;
    float width = 1.0f;
    float ypos = 0.95f;
    float height = 0.22f;

    DrawQuad(xpos, xpos + width, ypos, ypos - height, 1.0f);

    *text_start_x = xpos + 0.05f;
    *text_start_y = ypos - 0.04f;
    *text_width = 0.045f;
}
