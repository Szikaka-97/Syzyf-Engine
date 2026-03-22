#include "Text.h"
#include "Scene.h"
#include <glad/glad.h>

Text::Text(const std::string& text, float x, float y, float scale,
    const glm::vec3& color, Font* font, Material* material)
    : m_text(text)
    , m_x(x)
    , m_y(y)
    , m_scale(scale)
    , m_color(color)
    , m_font(font)
    , m_material(material) {
    setupBuffers();
}

Text::~Text() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}

void Text::setupBuffers() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Text::Render() {
    if (!m_material || !m_font) return;

    Scene* scene = GetScene();
    if (!scene) return;

    SceneGraphics* graphics = scene->GetGraphics();
    if (!graphics) return;

    glm::vec2 screenSize = graphics->GetScreenResolution();
    glm::mat4 ortho = glm::ortho(0.0f, screenSize.x, 0.0f, screenSize.y, -1.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderText(ortho);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Text::renderText(const glm::mat4& projection) {
    GLuint shaderID = m_material->GetShader()->GetHandle();
    if (!shaderID) return;

    m_material->Bind();

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3f(glGetUniformLocation(shaderID, "textColor"), m_color.r, m_color.g, m_color.b);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_VAO);

    float x = m_x;
    float y = m_y;
    float scale = m_scale;

    for (char c : m_text) {
        auto it = m_font->Characters.find(c);
        if (it == m_font->Characters.end()) continue;
        const Character& ch = it->second;

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Text::SetText(const std::string& newText) {
    m_text = newText;
}

void Text::SetPosition(float newX, float newY) {
    m_x = newX;
    m_y = newY;
}

void Text::SetScale(float newScale) {
    m_scale = newScale;
}

void Text::SetColor(const glm::vec3& newColor) {
    m_color = newColor;
}

void Text::SetFont(Font* newFont) {
    m_font = newFont;
}

void Text::SetMaterial(Material* newMaterial) {
    m_material = newMaterial;
}