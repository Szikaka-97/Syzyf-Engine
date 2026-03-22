#pragma once

#include <ft2build.h>
#include <iostream>
#include <glm/glm.hpp>
#include FT_FREETYPE_H

#include <Shader.h>
#include <Resources.h>
#include <map>
#include <glm/ext/matrix_clip_space.hpp>



struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

class Font : public Resource {
public:
    std::map<GLchar, Character> Characters;
    Font(std::string fontPath);
    Font(Font&&);
    Font& operator=(Font&&);
    ~Font() {
        for (auto& pair : Characters) {
            glDeleteTextures(1, &pair.second.TextureID);
        }
    }
private:

    std::string fontPath;


    void LoadFont();
};