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

        // Hierarchy
        //  This might break when moving the parent, dont care for now
        UiLayout* parentLayout = nullptr;
        SceneNode* parentNode = uiLayout->GetNode()->GetParent();
        if (parentNode) {
            parentLayout = parentNode->GetObject<UiLayout>();
        }

        float refX = 0.0f;
        float refY = 0.0f;
        float refWidth = resolution.x;
        float refHeight = resolution.y;

        if (parentLayout) {
            refX = parentLayout->finalRectangle.x;
            refY = parentLayout->finalRectangle.y;
            refWidth = parentLayout->finalRectangle.z;
            refHeight = parentLayout->finalRectangle.w;
        }

        float baseX = 0.0f;
        float baseY = 0.0f;
        switch (uiLayout->anchorPoint) {
            case AnchorPoint::TopLeft:
                break;
            case AnchorPoint::TopCenter:
                baseX = (refWidth * 0.5f) - (scaledWidth * 0.5f);
                break;
            case AnchorPoint::TopRight:
                baseX = refWidth - scaledWidth;
                break;
            case AnchorPoint::CenterLeft:
                baseY = (refHeight * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::Center:
                baseX = (refWidth * 0.5f) - (scaledWidth * 0.5f);
                baseY = (refHeight * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::CenterRight:
                baseX = refWidth - scaledWidth;
                baseY = (refHeight * 0.5f) - (scaledHeight * 0.5f);
                break;
            case AnchorPoint::BottomLeft:
                baseY = refHeight - scaledHeight;
                break;
            case AnchorPoint::BottomCenter:
                baseX = (refWidth * 0.5f) - (scaledWidth * 0.5f);
                baseY = refHeight - scaledHeight;
                break;
            case AnchorPoint::BottomRight:
                baseX = refWidth - scaledWidth;
                baseY = refHeight - scaledHeight;
                break;
            default:
                break;
        }

        float finalX = refX + baseX + scaledOffsetX;
        float finalY = refY + baseY + scaledOffsetY;

        uiLayout->finalRectangle = {
            finalX,
            finalY,
            scaledWidth,
            scaledHeight,
        };
    };
}
