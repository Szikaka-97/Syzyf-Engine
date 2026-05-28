#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "imgui.h"

class UiInteractable : public GameObject, public ImGuiDrawable {
public:
    bool isInteractable = true;
    bool isHovered = false;

    // Shows whether the button is held down
    bool isPressed = false;

    // Shows whether the button was pressed this frame
    bool isDown = false;

    // Placeholder until an event system exists(?)
    
    // Runs the first time user pressed the button
    std::function<void()> OnDown;
    std::function<void()> OnHoverEnter;
    std::function<void()> OnHoverExit;
    // Runs every frame while the button is being held down
    std::function<void()> OnPress;
    std::function<void()> OnRelease;

    void DrawImGui() {
        ImGui::Checkbox("Is Interactable", &this->isInteractable);
        ImGui::Checkbox("Is Hovered", &this->isHovered);
        ImGui::Checkbox("Is Pressed", &this->isPressed);
    } 
};
