#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "text/Font.h"
#include "ui/TextAlignment.h"

class UiText : public GameObject, public ImGuiDrawable {
public:
    serialized std::string text;
    serialized Font* font = nullptr;
    serialized glm::vec4 color{1.0f};
    serialized float fontSize = 32.0f;
    serialized TextAlignment alignment;

    // Max width until the text wraps around
    std::optional<float> maxWidth;

    UiText() = default;
    UiText(std::string text, Font* font);

    void DrawImGui();

    json Serialize() const;
    void Deserialize(const json& data);
};
