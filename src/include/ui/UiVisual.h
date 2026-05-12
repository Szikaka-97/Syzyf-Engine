#pragma once

#include "GameObject.h"
#include "Debug.h"
#include "Material.h"

class Texture2D;

class UiVisual : public GameObject, public ImGuiDrawable {
public:
    glm::vec4 color{1.0f};
    Texture2D* texture = nullptr;

    std::optional<glm::vec4> colorDisabled;
    Texture2D* textureDisabled = nullptr;
    std::optional<glm::vec4> colorHovered;
    Texture2D* textureHovered = nullptr;
    std::optional<glm::vec4> colorClicked;
    Texture2D* textureClicked = nullptr;

    Material* customMaterial = nullptr;

    UiVisual(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f}, Texture2D* texture = nullptr);

    void DrawImGui();
private:
    void DrawOptionalColor(const char* label, std::optional<glm::vec4>& color);
};
