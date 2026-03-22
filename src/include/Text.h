#pragma once

#include <ft2build.h>
#include <iostream>
#include <glm/glm.hpp>
#include FT_FREETYPE_H

#include <GameObject.h>
#include <Font.h>
#include <Material.h>
#include <Graphics.h>

#include <map>
#include <glm/ext/matrix_clip_space.hpp>

class Text : public GameObject {
public:
    Text(const std::string& text, float x, float y, float scale,
        const glm::vec3& color, Font* font, Material* material);
    ~Text();

    void Render();

    void SetText(const std::string& newText);
    void SetPosition(float newX, float newY);
    void SetScale(float newScale);
    void SetColor(const glm::vec3& newColor);
    void SetFont(Font* newFont);
    void SetMaterial(Material* newMaterial);

private:
    std::string m_text;
    float m_x;
    float m_y;
    float m_scale;
    glm::vec3 m_color;
    Font* m_font;
    Material* m_material;
    unsigned int m_VAO, m_VBO;

    void setupBuffers();
    void renderText(const glm::mat4& projection);
};