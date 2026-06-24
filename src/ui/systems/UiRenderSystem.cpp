#include "ui/systems/UiRenderSystem.h"
#include "Graphics.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/systems/UiLayoutSystem.h"

UiRenderSystem::UiRenderSystem(Scene* scene) : GameObjectSystem(scene) {}

void UiRenderSystem::OnPreRender() {
    auto* graphics = GetScene()->GetGraphics();
    if (!graphics) return;

    glm::vec2 resolution = graphics->GetScreenResolution();
    float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

    for (auto* visual : this->IterateObjects()) {
        UiLayout* layout = visual->GetNode()->GetObject<UiLayout>();
        UiInteractable* interactable = visual->GetNode()->GetObject<UiInteractable>();

        if (!layout) {
            spdlog::warn("UiRenderSystem: Tried rendering a UiVisual without a UiTransform component");
            continue;
        }

        if (!layout->GetNode()->IsEnabled()) {
            continue;
        }

        glm::vec4 finalColor = visual->color;
        Texture2D* finalTexture = visual->texture;

        // 0 -> Normal, 1 -> Hovered, 2 -> Pressed, 3 -> Disabled
        unsigned int interactionState = 0;
        if (interactable) {
            if (!interactable->isInteractable) {
                interactionState = 3;
                if (visual->colorDisabled.has_value()) finalColor = visual->colorDisabled.value();
                if (visual->textureDisabled) finalTexture = visual->textureDisabled;
            } else if (interactable->isPressed) {
                interactionState = 2;
                if (visual->colorClicked.has_value()) finalColor = visual->colorClicked.value();
                if (visual->textureClicked) finalTexture = visual->textureClicked;
            } else if (interactable->isHovered) {
                interactionState = 1;
                if (visual->colorHovered.has_value()) finalColor = visual->colorHovered.value();
                if (visual->textureHovered) finalTexture = visual->textureHovered;
            }
        }

        if (visual->customMaterial) {
            visual->customMaterial->SetValue("interactionState", interactionState);
        }

        glm::mat4 worldMatrix = layout->GlobalTransform().Value();
        glm::vec2 scaledSize = static_cast<glm::vec2>(layout->size) * scaleFactor;

        graphics->DrawUi(
            worldMatrix,
            scaledSize,
            layout->zIndex,
            finalColor,
            finalTexture,
            visual->customMaterial,
            visual->clipRectangle
        );
    }
}
