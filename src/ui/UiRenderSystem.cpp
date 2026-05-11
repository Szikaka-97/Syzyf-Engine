#include "ui/UiRenderSystem.h"
#include "Graphics.h"
#include "ui/UiLayout.h"

UiRenderSystem::UiRenderSystem(Scene* scene) : GameObjectSystem(scene) {}

void UiRenderSystem::OnPreRender() {
    auto* graphics = GetScene()->GetGraphics();
    if (!graphics) return;

    for (auto* visual : this->IterateObjects()) {
        UiLayout* layout = visual->GetNode()->GetObject<UiLayout>();

        if (!layout) {
            spdlog::warn("UiRenderSystem: Tried rendering a UiVisual without a UiTransform component");
            continue;
        }

        graphics->DrawUi(
            layout->finalRectangle,
            layout->zIndex,
            visual->color,
            visual->texture
        );
    }
}
