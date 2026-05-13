#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "text/Font.h"

class UiText : public GameObject, public ImGuiDrawable {
public:
    std::string text;
    Font* font = nullptr;
    glm::vec4 color{1.0f};
    float fontSize = 32.0f;

    UiText(std::string text = "", Font* font = nullptr);

    void DrawImGui();
};
