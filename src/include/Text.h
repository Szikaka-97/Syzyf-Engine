#pragma once

#include <ft2build.h>
#include <iostream>
#include <glm/glm.hpp>
#include FT_FREETYPE_H

#include <Shader.h>
#include <Font.h>
#include <map>
#include <glm/ext/matrix_clip_space.hpp>

class Text {

public:

    Text(std::string& text, float x, float y, float scale, glm::vec3 color, Font* fontPtr);

    void Draw(ShaderProgram& shader, glm::mat4 projection);

private:
    std::string text;
    float x;
    float y;
    float scale;
    glm::vec3 color;

    Font* font;
    unsigned int VAO, VBO;

    void setupBuffers();

    void RenderText(ShaderProgram& shader, std::string text, float x, float y, float scale, glm::vec3 color, glm::mat4 projection);


};
