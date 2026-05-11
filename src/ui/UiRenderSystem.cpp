#include "ui/UiRenderSystem.h"
#include "Graphics.h"
#include "ui/UiInteractable.h"
#include "ui/UiLayout.h"

UiRenderSystem::UiRenderSystem(Scene* scene) : GameObjectSystem(scene) {}

void UiRenderSystem::OnPreRender() {
    auto* graphics = GetScene()->GetGraphics();
    if (!graphics) return;

    for (auto* visual : this->IterateObjects()) {
        UiLayout* layout = visual->GetNode()->GetObject<UiLayout>();
        UiInteractable* interactable = visual->GetNode()->GetObject<UiInteractable>();

        if (!layout) {
            spdlog::warn("UiRenderSystem: Tried rendering a UiVisual without a UiTransform component");
            continue;
        }

        glm::vec4 finalColor = visual->color;
        Texture2D* finalTexture = visual->texture;

        if (interactable) {
            if (!interactable->isInteractable) {
                if (visual->colorDisabled.has_value()) finalColor = visual->colorDisabled.value();
                if (visual->textureDisabled) finalTexture = visual->textureDisabled;
            } else if (interactable->isPressed) {
                if (visual->colorClicked.has_value()) finalColor = visual->colorClicked.value();
                if (visual->textureClicked) finalTexture = visual->textureClicked;
            } else if (interactable->isHovered) {
                if (visual->colorHovered.has_value()) finalColor = visual->colorHovered.value();
                if (visual->textureHovered) finalTexture = visual->textureHovered;
            }
        }

        graphics->DrawUi(
            layout->finalRectangle,
            layout->zIndex,
            finalColor,
            finalTexture
        );
    }
}
