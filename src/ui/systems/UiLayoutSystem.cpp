#include "ui/systems/UiLayoutSystem.h"

#include "GameObjectSystem.h"
#include "Graphics.h"
#include "Transform.h"
#include "ui/objects/UiLayout.h"

UiLayoutSystem::UiLayoutSystem(Scene* scene) : GameObjectSystem<UiLayout>(scene) {}

void UiLayoutSystem::OnPreRender() {
    glm::vec2 resolution = this->GetScene()->GetGraphics()->GetScreenResolution();
    float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

    for (auto& uiLayout : this->IterateObjects()) {
        glm::vec2 scaledSize = static_cast<glm::vec2>(uiLayout->size) * scaleFactor;
        glm::vec2 scaledOffset = static_cast<glm::vec2>(uiLayout->offset) * scaleFactor;

        bool isRoot = true;
        glm::vec2 parentSize = resolution;
        if (SceneNode* parentNode = uiLayout->GetNode()->GetParent()) {
            if (auto* parentLayout = parentNode->GetObject<UiLayout>()) {
                parentSize = static_cast<glm::vec2>(parentLayout->size) * scaleFactor;
                isRoot = false;
            }
        }

        glm::vec2 localPosition = CalculateLocalAnchorPosition(uiLayout->anchorPoint, parentSize, scaledSize, scaledOffset);

        if (isRoot) {
            localPosition += resolution * 0.5f;
        }

        uiLayout->LocalTransform().Position() = { localPosition.x, localPosition.y, 0.0f };
    }
}

glm::vec2 UiLayoutSystem::CalculateLocalAnchorPosition(AnchorPoint anchor, glm::vec2 parentSize, glm::vec2 childSize, glm::vec2 offset) {
    float halfParentWidth = parentSize.x * 0.5f;
    float halfParentHeight = parentSize.y * 0.5f;
    float halfChildWidth = childSize.x * 0.5f;
    float halfChildHeight = childSize.y * 0.5f;

    glm::vec2 basePosition{0.0f};

    switch (anchor) {
        case AnchorPoint::TopLeft:
            basePosition = { -halfParentWidth + halfChildWidth, -halfParentHeight + halfChildHeight };
            break;
        case AnchorPoint::TopCenter:
            basePosition = { 0.0f, -halfParentHeight + halfChildHeight };
            break;
        case AnchorPoint::TopRight:
            basePosition = { halfParentWidth - halfChildWidth, -halfParentHeight + halfChildHeight };
            break;
        case AnchorPoint::CenterLeft:
            basePosition = { -halfParentWidth + halfChildWidth, 0.0f };
            break;
        case AnchorPoint::Center:
            basePosition = { 0.0f, 0.0f };
            break;
        case AnchorPoint::CenterRight:
            basePosition = { halfParentWidth - halfChildWidth, 0.0f };
            break;
        case AnchorPoint::BottomLeft:
            basePosition = { -halfParentWidth + halfChildWidth,  halfParentHeight - halfChildHeight };
            break;
        case AnchorPoint::BottomCenter:
            basePosition = { 0.0f, halfParentHeight - halfChildHeight };
            break;
        case AnchorPoint::BottomRight:
            basePosition = { halfParentWidth - halfChildWidth, halfParentHeight - halfChildHeight }; 
            break;
    }

    return basePosition + offset;
}
