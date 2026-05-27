#include "ui/objects/UiScrollableGrid.h"
#include "TimeSystem.h"
#include "InputSystem.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiVisual.h"
#include "Graphics.h"
#include "ui/systems/UiLayoutSystem.h"

#include <imgui.h>

void UiScrollableGrid::Update() {
    auto* input = GetScene()->Input();
    auto* layout = GetNode()->GetObject<UiLayout>();
    if (!input || !layout) return;

    glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
    float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

    glm::vec3 containerPosition = layout->GetNode()->GlobalTransform().Position();
    glm::vec2 scaledContainerSize = static_cast<glm::vec2>(layout->size) * scaleFactor;

    float scissorW = std::round(scaledContainerSize.x);
    float scissorH = std::round(scaledContainerSize.y);
    float scissorX = std::round(containerPosition.x - (scaledContainerSize.x / 2.0f));
    float scissorTop = std::round(containerPosition.y - (scaledContainerSize.y / 2.0f));
    float scissorY = std::round(resolution.y - (scissorTop + scissorH));

    glm::vec4 clipBox(scissorX, scissorY, scissorW, scissorH);

    std::vector<UiLayout*> gridItems;
    for (auto* childNode : GetNode()->GetChildren()) {
        if (auto* childLayout = childNode->GetObject<UiLayout>()) {
            gridItems.push_back(childLayout);
        }
    }
    
    if (gridItems.empty()) return;

    int col = selectedIndex % columns;
    maxIndex = static_cast<int>(gridItems.size()) - 1;

    if (input->KeyDown(Key::Right) && col < columns - 1 && selectedIndex < maxIndex) {
        selectedIndex++;
    }
    if (input->KeyDown(Key::Left) && col > 0) {
        selectedIndex--;
    }
    if (input->KeyDown(Key::Down) && selectedIndex + columns <= maxIndex) {
        selectedIndex += columns;
    }
    if (input->KeyDown(Key::Up) && selectedIndex - columns >= 0) {
        selectedIndex -= columns;
    }

    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex >= gridItems.size()) selectedIndex = gridItems.size() - 1;

    float rowHeight = static_cast<float>(cellSize.y + spacing.y);
    int selectedRow = selectedIndex / columns;

    float selectedTopEdge = selectedRow * rowHeight;
    float selectedBottomEdge = selectedTopEdge + cellSize.y;

    if (selectedTopEdge < targetScrollY) {
        targetScrollY = selectedTopEdge;
    } else if (selectedBottomEdge > targetScrollY + layout->size.y) {
        float neededScroll = selectedBottomEdge - layout->size.y;
        targetScrollY = std::ceil(neededScroll / rowHeight) * rowHeight;
    }

    scrollY += (targetScrollY - scrollY) * scrollSpeed * Time::UnscaledDelta();

    int actualColumns = std::min((int)gridItems.size(), columns);
    float totalGridWidth = (actualColumns * cellSize.x) + ((actualColumns - 1) * spacing.x);
    
    float startX = -(totalGridWidth / 2.0f) + (cellSize.x / 2.0f);
    float startY = -(layout->size.y / 2.0f) + (cellSize.y / 2.0f);

    for (size_t i = 0; i < gridItems.size(); i++) {
        UiLayout* childLayout = gridItems[i];
        SceneNode* childNode = childLayout->GetNode();

        int row = i / columns;
        int col = i % columns;

        float targetX = startX + col * (cellSize.x + spacing.x);
        float targetY = startY + row * rowHeight - scrollY;

        childLayout->offset = glm::ivec2(static_cast<int>(targetX), static_cast<int>(targetY));

        if (auto* visual = childNode->GetObject<UiVisual>()) {
            visual->clipRectangle = clipBox;
            float currentAlpha = visual->color.a;

            if (i == selectedIndex) {
                visual->color = glm::vec4(0.0f, 1.0f, 0.0f, currentAlpha);
            } else {
                visual->color = glm::vec4(1.0f, 0.0f, 0.0f, currentAlpha);
            }
        }

        float topEdge = targetY - (cellSize.y / 2.0f);
        float bottomEdge = targetY + (cellSize.y / 2.0f);
        float halfContainerHeight = layout->size.y / 2.0f;

        bool isVisible = (bottomEdge > -halfContainerHeight) && (topEdge < halfContainerHeight);
        childNode->SetEnabled(isVisible);
    }
}

void UiScrollableGrid::ScrollUp() {
    if (selectedIndex - columns >= 0) {
        selectedIndex -= columns;
    }
}

void UiScrollableGrid::ScrollDown() {
    if (selectedIndex + columns <= maxIndex) {
        selectedIndex += columns; 
    } 
}

void UiScrollableGrid::DrawImGui() {
    ImGui::InputInt("Columns", &columns);
    ImGui::InputInt2("Cell Size", &cellSize[0]);
    ImGui::InputInt2("Spacing", &spacing[0]);
    ImGui::DragFloat("Lerp Speed", &scrollSpeed);
}


