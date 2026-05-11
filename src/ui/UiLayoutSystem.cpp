#include "ui/UiLayoutSystem.h"

#include "GameObjectSystem.h"
#include "Graphics.h"
#include "ui/UiLayout.h"

UiLayoutSystem::UiLayoutSystem(Scene* scene) : GameObjectSystem<UiLayout>(scene) {}

void UiLayoutSystem::OnPreRender() {
    glm::vec2 resolution = this->GetScene()->GetGraphics()->GetScreenResolution();
    float scaleFactor = resolution.y / this->VIRTUAL_RESOLUTION.y;

    for (auto& uiLayout : this->IterateObjects()) {
        float scaledWidth = uiLayout->size.x * scaleFactor;
        float scaledHeight = uiLayout->size.y * scaleFactor;
        float scaledOffsetX = uiLayout->offset.x * scaleFactor;
        float scaledOffsetY = uiLayout->offset.y * scaleFactor;

        float baseX = 0.0f;
        float baseY = 0.0f;
        switch (uiLayout->anchorPoint) {
            case AnchorPoint::TopLeft:
                break;
            case AnchorPoint::TopCenter:
                baseX = (resolution.x * 0.5f) - (scaledWidth * 0.5f);
                break;
            case AnchorPoint::TopRight:
                baseX = resolution.x - scaledWidth;
                break;
            case AnchorPoint::CenterLeft:
                baseY = (resolution.y * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::Center:
                baseX = (resolution.x * 0.5f) - (scaledWidth * 0.5f);
                baseY = (resolution.y * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::CenterRight:
                baseX = resolution.x - scaledWidth;
                baseY = (resolution.y * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::BottomLeft:
                baseY = resolution.y - scaledHeight;
                break;
            case AnchorPoint::BottomCenter:
                baseX = (resolution.x * 0.5f) - (scaledWidth * 0.5f);
                baseY = resolution.y - scaledHeight;
                break;
            case AnchorPoint::BottomRight:
                baseX = resolution.x - scaledWidth;
                baseY = resolution.y - scaledHeight;
                break;
            default:
                break;
        }

        float finalX = baseX + scaledOffsetX;
        float finalY = baseY + scaledOffsetY;

        uiLayout->finalRectangle = {
            finalX,
            finalY,
            scaledWidth,
            scaledHeight,
        };
    };
}
