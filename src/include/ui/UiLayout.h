#pragma once

#include "Debug.h"
#include "GameObject.h"

enum class AnchorPoint {
    TopLeft,
    TopRight,
    TopCenter,
    CenterLeft,
    Center,
    CenterRight,
    BottomCenter,
    BottomLeft,
    BottomRight,
};

class UiLayout : public GameObject, public ImGuiDrawable {
public:
    AnchorPoint anchorPoint = AnchorPoint::TopLeft;
    glm::ivec2 offset{0};
    glm::ivec2 size{0};

    int zIndex = 0;

    glm::vec4 finalRectangle{0.0f};
public:
    UiLayout(glm::ivec2 size = {0, 0}, glm::ivec2 offset = {0, 0}, int zIndex = 0, AnchorPoint anchorPoint = AnchorPoint::TopLeft);

    void DrawImGui();
private:
    void DrawAnchorButton(const char* label, AnchorPoint point);
};
