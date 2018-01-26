#include "stdafx.h"
#include "TextRenderer.h"
#include "Configuration.h"
#include "chockNgt.h"

// Hard coded addresses into the font texture (called gothic_64x64_48pt.png)
#include "font.h"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

void Script::Load(std::string filename) {
    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line)) {
        float a, b;
        int c;
        std::string text;

        // remove BOM
        const char bom[4] = {-17, -69, -65, 0};
        std::string from = bom;
        std::string to = "";
        size_t start_pos = 0;
        if ((start_pos = line.find(from, start_pos)) != std::string::npos) {
            if (start_pos == 0) {
                line.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
        }

        // Change , to . for German style decimals
        // replace Japanese whitespace
        from = ",";
        to = ".";
        start_pos = 0;
        while ((start_pos = line.find(from, start_pos)) != std::string::npos) {
            line.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }

        // Fake the Japanese lengthener
        from = "ー";
        to = "－";
        start_pos = 0;
        while ((start_pos = line.find(from, start_pos)) != std::string::npos) {
            line.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }

        // replace Japanese whitespace
        from = "　";
        to = " ";
        start_pos = 0;
        while ((start_pos = line.find(from, start_pos)) != std::string::npos) {
            line.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }

        std::istringstream iss(line);
        if (iss >> a >> b >> c >> text) {
            start_time_.push_back(a);
            end_time_.push_back(b);
            color_index_.push_back(c);
            text_.push_back(text);
        } else {
            std::istringstream iss2(line);
            if (iss2 >> a >> b >> text) {
                start_time_.push_back(a);
                end_time_.push_back(b);
                color_index_.push_back(0);
                text_.push_back(text);
            }
        }
    }
};

TextRenderer::TextRenderer() {
    int num_characters = sizeof(font_x_) / sizeof(font_x_[0]);

    // Create the map from character to coordinate
    for (int i = 0; i < num_characters; i++) {
        std::string character = font_char_[i];
        font_x_map_[character] = font_x_[i];
        font_y_map_[character] = font_y_[i];
    }

    // Check something
    const char *text_char = "豊";
    int x = GetCharacterX(text_char);
    int y = GetCharacterY(text_char);

    // Go throught the shaders directory and load all shaders.
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;

    // Go to first file in scripts directory
    hFind = FindFirstFile(SCRIPT_DIRECTORY SCRIPT_WILDCARD, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        // PANIC
        return;
    }

    // Load all the scripts in the directory
    do {
        Script script;
        scripts_.push_back(script);
        script_names_.push_back(ffd.cFileName);
        int num_scripts = GetNumScripts();
        LoadScript(ffd.cFileName, &(scripts_[num_scripts-1]));
    } while (FindNextFile(hFind, &ffd));

    // Check loading some text
    int color_index;
    std::string text = GetScriptText("example_script.txt", 7.5f, &color_index);
}

TextRenderer::~TextRenderer() {
}

void TextRenderer::LoadScript(std::string filename, Script *result) {
    result->Load(SCRIPT_DIRECTORY + filename);
}

int TextRenderer::GetNumCharacters(const unsigned char *text) {
	int num_chars = 0;

	while (*text != 0 && *text != '/') {
		if (text[0] < 128) {
			text++;
		} else {
			if ((text[0] & 0b11110000) == 0b11110000) {
				text += 4;
			} else {
				if ((text[0] & 0b11100000) == 0b11100000) {
					text += 3;
				} else {
					if ((text[0] & 0b11000000) == 0b11000000) {
						text += 2;
					} else {
						// Some error?
						text++;
					}
				}
			}
		}
		num_chars++;
	}
	return num_chars;
}

float TextRenderer::GetXFeed(const unsigned char *text, float max_characters_per_line, float size) {
	int num_chars = GetNumCharacters(text);
	float empty_boxes = max_characters_per_line - static_cast<float>(num_chars);
	float left_boxes = empty_boxes * 0.5f;
	return left_boxes * size;
}

void TextRenderer::RenderText(float x, float y, float width, float height,
                              const char *script_name, float time,
                              TextureManager *texture_manager, float max_characters_per_line) {
    GLuint tex_id;
    char error_string[MAX_ERROR_LENGTH + 1];
    if (texture_manager) {
        texture_manager->getTextureID(SCRIPT_FONT_TEXTURE, &tex_id, error_string);
        glBindTexture(GL_TEXTURE_2D, tex_id);
    }

    int color_index = 0;
    std::string text = GetScriptText(script_name, time, &color_index);

    // Check whether there is a line separator in the script
    size_t line_separator_pos = text.find_first_of("/");
    if (line_separator_pos == std::string::npos) {
        y -= height / 2.0f * ASPECT_RATIO;
    }

    // Iterate over the text
    const char *current_pos_signed = text.c_str();
    const unsigned char *current_pos = reinterpret_cast<const unsigned char *>(current_pos_signed);
	float current_x = x + GetXFeed(current_pos, max_characters_per_line, width);

    while (true) {
        char character[5];  // Maximum UTF-8 size is 4 Bytes

        if (current_pos[0] < 128) {
            character[0] = current_pos[0];
            character[1] = 0;
            current_pos++;
        } else {
            if ((current_pos[0] & 0b11110000) == 0b11110000) {
                character[0] = current_pos[0];
                character[1] = current_pos[1];
                character[2] = current_pos[2];
                character[3] = current_pos[3];
                character[4] = 0;
                current_pos += 4;
            } else {
                if ((current_pos[0] & 0b11100000) == 0b11100000) {
                    character[0] = current_pos[0];
                    character[1] = current_pos[1];
                    character[2] = current_pos[2];
                    character[3] = 0;
                    current_pos += 3;
                } else {
                    if ((current_pos[0] & 0b11000000) == 0b11000000) {
                        character[0] = current_pos[0];
                        character[1] = current_pos[1];
                        character[2] = 0;
                        current_pos += 2;
                    } else {
                        // Some error?
                        character[0] = 0;
                        current_pos++;
                    }
                }
            }
        }
        if (0 == character[0]) break;  // It's all processed

        if ('/' == character[0]) {
            // Go to next line
			current_x = x + GetXFeed(current_pos, max_characters_per_line, width);
            y -= height * ASPECT_RATIO;
        } else {
            int xi = GetCharacterX(character);
            int yi = GetCharacterY(character);

            // Calculate texture coordinates
            float texture_step = 1.0f / SCRIPT_FONT_TEXTURE_RESOLUTION;
            float texture_half_pixel = 0.5f / SCRIPT_FONT_TEXTURE_PIXELS;
            float tx = xi * texture_step;
            float ty = yi * texture_step;
            float txl = tx + texture_half_pixel;
            float txr = tx + texture_step - texture_half_pixel;
            float tyt = ty + texture_half_pixel;
            float tyb = ty + texture_step - texture_half_pixel;

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            switch (color_index) {
            case 1:
                r = 0.5f, g = 0.1f, b = 0.1f;
                break;
            case 2:
                r = 0.1f, g = 0.5f, b = 0.1f;
                break;
            case 3:
                r = 0.1f, g = 0.1f, b = 0.5f;
                break;
            case 4:
                r = 0.4f, g = 0.4f, b = 0.1f;
                break;
            case 5:
                r = 0.1f, g = 0.4f, b = 0.4f;
                break;
            case 6:
                r = 0.4f, g = 0.1f, b = 0.4f;
                break;
            }

            // Render a quad..
            DrawQuadColor(current_x, current_x + width, y, y - height * ASPECT_RATIO,
                          txl, txr, tyt, tyb, r, g, b, 1.0f);
            current_x += width;  // Go to next character
        }
    }
    glEnd();
}