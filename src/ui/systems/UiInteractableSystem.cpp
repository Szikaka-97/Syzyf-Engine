#include "ui/systems/UiInteractableSystem.h"
#include "InputSystem.h"
#include "imgui.h"
#include "ui/objects/UiInteractable.h"
#include "Graphics.h"
#include "ui/objects/UiLayout.h"
#include "ui/systems/UiLayoutSystem.h"

UiInteractableSystem::UiInteractableSystem(Scene* scene) : GameObjectSystem<UiInteractable>(scene) {}

void UiInteractableSystem::OnPreUpdate() {
    auto input = GetScene()->GetComponent<InputSystem>();
    if (!input) return;

    glm::vec2 mousePosition = input->GetMousePosition();
    bool mouseDown = input->ButtonDown(MouseButton::Left);
    bool mousePressed = input->ButtonPressed(MouseButton::Left);
    bool mouseReleased = input->ButtonUp(MouseButton::Left);

    const glm::vec2 resolution = this->GetScene()->GetGraphics()->GetScreenResolution();
    const float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

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

        bool wasHovered = interactable->isHovered;
        bool wasPressed = interactable->isPressed;

        const glm::mat4 worldMatrix = layout->GlobalTransform().Value();
        const glm::vec2 scaledSize = static_cast<glm::vec2>(layout->size) * scaleFactor;

        glm::mat4 modelMatrix = glm::scale(worldMatrix, glm::vec3(scaledSize.x, scaledSize.y, 1.0f));
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, -0.5f, 0.0f));

        const glm::mat4 inverseModel = glm::inverse(modelMatrix);

        const glm::vec4 localMouse = inverseModel * glm::vec4(mousePosition.x, mousePosition.y, 0.0f, 1.0f);

        const bool isMouseOver = !inputConsumed &&
            localMouse.x >= 0.0f && localMouse.x <= 1.0f &&
            localMouse.y >= 0.0f && localMouse.y <= 1.0f;

        if (isMouseOver) {
            interactable->isHovered = true;
            inputConsumed = true;

            if (!wasHovered && interactable->OnHoverEnter) interactable->OnHoverEnter();

            if (mouseDown) {
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
