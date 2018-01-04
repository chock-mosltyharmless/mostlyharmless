#pragma once

#include "TextureManager.h"

#include <map>
#include <vector>

#define SCRIPT_DIRECTORY "scripts/"
#define SCRIPT_WILDCARD "*.txt"
#define SCRIPT_FONT_TEXTURE "gothic_64x64_48pt_alpha.png"
// Matrix width/height of the font texture
#define SCRIPT_FONT_TEXTURE_RESOLUTION 64
// Resultion of the font texture in pixels
#define SCRIPT_FONT_TEXTURE_PIXELS 4096

// Class containing one translation
struct Script {
public:
    int GetNumLines() const {
        return text_.size();
    }
    // Returns the first one only
    std::string GetText(float time, int *color_index) const {
        for (int i = 0; i < GetNumLines(); i++) {
            if (time >= start_time_[i] && time <= end_time_[i]) {
                *color_index = color_index_[i];
                return text_[i];
            }
        }
        // No text --> return empty string
        return "";
    }
    void Load(std::string filename);
private:
    std::vector<float>start_time_;
    std::vector<float>end_time_;
    std::vector<int>color_index_;
    std::vector<std::string>text_;
};

class TextRenderer
{
public:
    TextRenderer();
    virtual ~TextRenderer();

    // use OpenGL coordinates and sizes
    void RenderText(float x, float y, float size, const char *script_name, float time,
                    TextureManager *texture_manager);

private:
    // Returns 0 on failure
    int GetCharacterX(std::string character) {
        std::map<std::string, int>::iterator it = font_x_map_.find(character);
        if (it == font_x_map_.end()) {
            // not found
            return SCRIPT_FONT_TEXTURE_RESOLUTION - 1;
        } else {
            return it->second;
        }
    }
    // Returns 0 on failure
    int GetCharacterY(std::string character) {
        std::map<std::string, int>::iterator it = font_y_map_.find(character);
        if (it == font_y_map_.end()) {
            // not found
            return SCRIPT_FONT_TEXTURE_RESOLUTION - 1;
        }
        else {
            return it->second;
        }
    }

    int GetNumScripts() const {
        return scripts_.size();
    }

    // Returns "" if there is no text for this script at that time
    std::string GetScriptText(const char *script_name, float time, int *color_index) const {
        for (int i = 0; i < GetNumScripts(); i++) {
            if (script_names_[i].compare(script_name) == 0) {
                return scripts_[i].GetText(time, color_index);
            }
        }
        return "";  // No such script found...
    }

    void TextRenderer::LoadScript(std::string filename, Script *result);

    std::map<std::string, int> font_x_map_;
    std::map<std::string, int> font_y_map_;
    std::vector<Script> scripts_;
    std::vector<std::string> script_names_;
};

