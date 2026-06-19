#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/widgets/wheel/UiWheel.h"
#include <Scene.h>
#include <InputSystem.h>
#include <TimeSystem.h>
#include <Resources.h>
#include <Graphics.h>
#include <ui/systems/UiLayoutSystem.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <vector>
#include <string>
#include <algorithm>

struct ScrollingListItemData {
    std::string text;
    Texture2D* icon;
};

class ScrollingList : public GameObject {
public:
    std::vector<SceneNode*> itemNodes;
    std::vector<UiInteractable*> itemInteractables;
    UiInteractable* scrollbarHandle = nullptr;
    SceneNode* scrollbarHandleNode = nullptr;
    UiLayout* layout = nullptr;

    int selectedIndex = 0;
    float scrollY = 0.0f;
    float targetScrollY = 0.0f;
    
    float itemHeight = 80.0f;
    float spacing = 10.0f;
    float scrollSpeed = 15.0f;
    float handleHeight = 100.0f;

    glm::vec4 colorNormal = glm::vec4(0.2f, 0.2f, 0.2f, 0.8f);
    glm::vec4 colorSelected = glm::vec4(0.4f, 0.6f, 0.4f, 1.0f);

    ScrollingList() = default;

    void Initialize(Font* font, const std::vector<ScrollingListItemData>& itemsData) {
        Scene* mainScene = GetScene();
        SceneNode* listRoot = GetNode();
        
        this->layout = listRoot->AddObjectIfMissing<UiLayout>(
            glm::uvec2(400, 860), glm::ivec2(-40, 40), 10, AnchorPoint::CenterRight);

        float listWidth = 340.0f;

        for (size_t i = 0; i < itemsData.size(); i++) {
            SceneNode* itemNode = mainScene->GetOrCreateNode(listRoot, "Item_" + std::to_string(i));
            
            itemNode->AddObjectIfMissing<UiLayout>(
                glm::uvec2(listWidth, itemHeight), glm::ivec2(0, 0), 11, AnchorPoint::Center);
            itemNode->AddObjectIfMissing<UiVisual>(colorNormal);
            itemNode->AddObjectIfMissing<WheelTag>();
            
            auto* interactable = itemNode->AddObjectIfMissing<UiInteractable>();
            
            itemNodes.push_back(itemNode);
            itemInteractables.push_back(interactable);

            SceneNode* iconNode = mainScene->GetOrCreateNode(itemNode, "Icon_" + std::to_string(i));
            iconNode->AddObjectIfMissing<UiLayout>(
                glm::uvec2(itemHeight - 10, itemHeight - 10), 
                glm::ivec2(5, 0), 12, AnchorPoint::CenterLeft);
            iconNode->AddObjectIfMissing<WheelTag>();
                
            if (itemsData[i].icon) {
                iconNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f), itemsData[i].icon);
            } else {
                iconNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            }

            SceneNode* textContainerNode = mainScene->GetOrCreateNode(itemNode, "TextContainer_" + std::to_string(i));
            textContainerNode->AddObjectIfMissing<UiLayout>(
                glm::uvec2(listWidth - itemHeight - 20, itemHeight), 
                glm::ivec2(itemHeight + 10, 0), 12, AnchorPoint::CenterLeft);

            SceneNode* textNode = mainScene->GetOrCreateNode(textContainerNode, "Text_" + std::to_string(i));
            textNode->AddObjectIfMissing<UiLayout>(
                glm::uvec2(listWidth - itemHeight - 20, itemHeight), 
                glm::ivec2(0, 0), 13, AnchorPoint::Center);
            textNode->AddObjectIfMissing<WheelTag>();
                
            auto* textComp = textNode->AddObjectIfMissing<UiText>(itemsData[i].text, font);
            textComp->fontSize = 24.0f;
        }

        SceneNode* scrollbarTrackNode = mainScene->GetOrCreateNode(listRoot, "Scrollbar Track");
        scrollbarTrackNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(20, 860), glm::ivec2(0, 0), 11, AnchorPoint::CenterRight);
        scrollbarTrackNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));
        scrollbarTrackNode->AddObjectIfMissing<WheelTag>();

        scrollbarHandleNode = mainScene->GetOrCreateNode(scrollbarTrackNode, "Scrollbar Handle");
        scrollbarHandleNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(20, handleHeight), glm::ivec2(0, 0), 12, AnchorPoint::Center);
        scrollbarHandleNode->AddObjectIfMissing<WheelTag>();
        
        auto* handleVisual = scrollbarHandleNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        handleVisual->colorHovered = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
        
        scrollbarHandle = scrollbarHandleNode->AddObjectIfMissing<UiInteractable>();
    }

    void Update() {
        auto* input = GetScene()->GetComponent<InputSystem>();
        if (!input || !layout || itemNodes.empty()) return;

        int maxIndex = static_cast<int>(itemNodes.size()) - 1;

        if (input->KeyDown(Key::Down) && selectedIndex < maxIndex) {
            selectedIndex++;
        }
        if (input->KeyDown(Key::Up) && selectedIndex > 0) {
            selectedIndex--;
        }

        selectedIndex = std::clamp(selectedIndex, 0, maxIndex);

        for (size_t i = 0; i < itemNodes.size(); i++) {
            if (itemInteractables[i]->isHovered) {
                selectedIndex = i;
            }
        }

        float rowHeight = itemHeight + spacing;
        float selectedTopEdge = selectedIndex * rowHeight;
        float selectedBottomEdge = selectedTopEdge + itemHeight;
        float visibleHeight = static_cast<float>(layout->size.y);
        float totalHeight = itemNodes.size() * rowHeight - spacing;
        float maxScroll = std::max(0.0f, totalHeight - visibleHeight);

        if (selectedTopEdge < targetScrollY) {
            targetScrollY = selectedTopEdge;
        } else if (selectedBottomEdge > targetScrollY + visibleHeight) {
            targetScrollY = selectedBottomEdge - visibleHeight;
        }

        if (scrollbarHandle && scrollbarHandle->isPressed) {
            glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
            float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;
            float virtualMouseY = input->GetMousePosition().y / scaleFactor;
            
            float relativeMouseY = virtualMouseY - (UiLayoutSystem::VIRTUAL_RESOLUTION.y / 2.0f);
            
            float trackTop = -visibleHeight / 2.0f + handleHeight / 2.0f;
            float trackBot = visibleHeight / 2.0f - handleHeight / 2.0f;
            
            float fraction = (relativeMouseY - trackTop) / (trackBot - trackTop);
            fraction = std::clamp(fraction, 0.0f, 1.0f);
            
            targetScrollY = fraction * maxScroll;
            scrollY = targetScrollY;
        } else {
            scrollY += (targetScrollY - scrollY) * scrollSpeed * Time::UnscaledDelta();
        }

        targetScrollY = std::clamp(targetScrollY, 0.0f, maxScroll);

        float startY = -visibleHeight / 2.0f + itemHeight / 2.0f;

        for (size_t i = 0; i < itemNodes.size(); i++) {
            SceneNode* childNode = itemNodes[i];
            UiLayout* childLayout = childNode->GetObject<UiLayout>();
            
            float targetY = startY + (i * rowHeight) - scrollY;
            childLayout->offset = glm::ivec2(0, static_cast<int>(targetY));

            if (auto* visual = childNode->GetObject<UiVisual>()) {
                visual->color = (i == selectedIndex) ? colorSelected : colorNormal;
            }
        }

        if (scrollbarHandleNode && maxScroll > 0.0f) {
            float handleFraction = scrollY / maxScroll;
            float handleRange = visibleHeight - handleHeight;
            float handleY = -visibleHeight / 2.0f + handleHeight / 2.0f + (handleFraction * handleRange);
            
            if (auto* handleLayout = scrollbarHandleNode->GetObject<UiLayout>()) {
                handleLayout->offset = glm::ivec2(0, static_cast<int>(handleY));
            }
        }
    }
};
