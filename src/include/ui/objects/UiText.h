#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "text/Font.h"
#include "ui/TextAlignment.h"

class UiText : public GameObject, public ImGuiDrawable {
public:
    std::string text;
    Font* font = nullptr;
    glm::vec4 color{1.0f};
    float fontSize = 32.0f;
    TextAlignment alignment;

    // Max width until the text wraps around
    std::optional<float> maxWidth;

    UiText(std::string text = "", Font* font = nullptr);

    void DrawImGui();
};
