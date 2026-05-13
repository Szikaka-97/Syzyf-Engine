#pragma once

#include "Debug.h"
#include "GameObject.h"

class UiInteractable : public GameObject, public ImGuiDrawable {
public:
    bool isInteractable = true;
    bool isHovered = false;
    bool isPressed = false;

    // Placeholder until an event system exists(?)
    std::function<void()> OnClick;
    std::function<void()> OnHoverEnter;
    std::function<void()> OnHoverExit;
    std::function<void()> OnPress;
    std::function<void()> OnRelease;

    void DrawImGui();
};
