#pragma once

#include "GameObject.h"
#include "Debug.h"

class Texture2D;

class UiVisual : public GameObject, public ImGuiDrawable {
public:
    glm::vec4 color{1.0f};
    Texture2D* texture = nullptr;

    UiVisual(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f}, Texture2D* texture = nullptr);

    void DrawImGui();
};
