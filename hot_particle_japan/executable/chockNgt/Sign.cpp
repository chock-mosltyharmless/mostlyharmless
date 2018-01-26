#include "stdafx.h"
#include "Sign.h"

#include "Configuration.h"
#include "chockNgt.h"

Sign::Sign() {
}


Sign::~Sign() {
}

void Sign::Draw(float time, TextureManager * texture_manager,
                float *text_start_x, float *text_start_y,
                float *text_width, float *text_height) {
    GLuint tex_id;
    char error_string[MAX_ERROR_LENGTH + 1];
    
    float xpos = -0.5f;
    float width = 1.0f;
    float ypos = 0.95f;
    float height = 0.22f;

    *text_start_x = xpos + 0.05f;

    float rotate_stretch = 1.0f;
    if (time > 0.0f) {
        ypos = 1.01f - time * 0.3f;
        if (ypos < 0.95f) ypos = 0.95f;
        float swing_amount = 0.0f;
        if (time < 3.14f * 0.75f) {
            swing_amount = 0.5f + 0.5f * cosf(time * 1.5f);
        }
        rotate_stretch = (cosf(3.14f * 0.5f * cosf(time * 4.0f) * swing_amount));
        height = height * rotate_stretch;
    }

    if (time < -10.0f) {
        *text_start_y = 2.0f;
        *text_width = 0.0f;
        return;
    }
    if (time < 0.0f) {
        ypos -= time * 0.4f;
    }

    texture_manager->getTextureID(SIGN_TEXTURE, &tex_id, error_string);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(xpos, xpos + width, ypos, ypos - height, 1.0f);
    
    texture_manager->getTextureID(STRINGS_TEXTURE, &tex_id, error_string);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    DrawQuad(xpos-0.006f, xpos + width-0.007f, ypos - 0.055f * rotate_stretch, ypos + 0.2f, 1.0f);

    *text_start_y = ypos - 0.04f * rotate_stretch;
    *text_width = 0.045f;
    *text_height = 0.045f * rotate_stretch;
}
