#include "ui/UiInteractableSystem.h"
#include "InputSystem.h"
#include "imgui.h"
#include "ui/UiInteractable.h"
#include "ui/UiLayout.h"

UiInteractableSystem::UiInteractableSystem(Scene* scene) : GameObjectSystem<UiInteractable>(scene) {}

void UiInteractableSystem::OnPreUpdate() {
    auto input = GetScene()->GetComponent<InputSystem>();
    if (!input) return;

    glm::vec2 mousePosition = input->GetMousePosition();
    bool mouseDown = input->ButtonDown(MouseButton::Left);
    bool mousePressed = input->ButtonPressed(MouseButton::Left);
    bool mouseReleased = input->ButtonUp(MouseButton::Left);

    std::vector<UiInteractable*> active;
    for (auto& interactable : this->IterateObjects()) {
        if (interactable->IsEnabled() && interactable->isInteractable) {
            active.push_back(interactable);
        } else {
            interactable->isHovered = false;
            interactable->isPressed = false;
        }
    }

    std::sort(active.begin(), active.end(), [](UiInteractable* a, UiInteractable* b) {
        auto layoutA = a->GetNode()->GetObject<UiLayout>();
        auto layoutB = b->GetNode()->GetObject<UiLayout>();
        int zA = layoutA ? layoutA->zIndex : 0;
        int zB = layoutB ? layoutB->zIndex : 0;
        return zA > zB;
    });

    bool inputConsumed = false;

    for (auto* interactable : active) {
        auto layout = interactable->GetNode()->GetObject<UiLayout>();
        if (!layout) continue;

        glm::vec4 rectangle = layout->finalRectangle;

        bool wasHovered = interactable->isHovered;
        bool wasPressed = interactable->isPressed;

        bool isMouseOver = !inputConsumed &&
            mousePosition.x >= rectangle.x && mousePosition.x <= rectangle.x + rectangle.z &&
            mousePosition.y >= rectangle.y && mousePosition.y <= rectangle.y + rectangle.w;

        if (isMouseOver) {
            interactable->isHovered = true;
            inputConsumed = true;

            if (!wasHovered && interactable->OnHoverExit) interactable->OnHoverEnter();

            if (mousePressed) {
                interactable->isPressed = true;
                if (interactable->OnPress) interactable->OnPress();
            } else if (mouseReleased && wasPressed) {
                interactable->isPressed = false;
                if (interactable->OnClick) interactable->OnClick();
                if (interactable->OnRelease) interactable->OnRelease();
            } else if (!mouseDown) {
                interactable->isPressed = false;
            }
        } else {
            interactable->isHovered = false;

            if (mouseReleased && wasPressed) {
                interactable->isPressed = false;
                if (interactable->OnRelease) interactable->OnRelease();
            } else if (!mouseDown) {
                interactable->isPressed = false;
            }

            if (wasHovered && interactable->OnHoverExit) interactable->OnHoverExit();
        }
    }
}

void UiInteractable::DrawImGui() {
    ImGui::Checkbox("Interactable", &this->isInteractable);
}
