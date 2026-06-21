#pragma once

#include "Application.h"
#include "GameObject.h"
#include "ui/TextAlignment.h"
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
#include <ui/widgets/wheel/UiWheel.h>

struct ScrollingListItemData {
    std::string text;
    int count;
    Texture2D* icon;
};

class ScrollingList : public GameObject {
public:
    Font* listFont = nullptr;
    int activeItemCount = 0;

    std::vector<SceneNode*> itemNodes;
    std::vector<UiInteractable*> itemInteractables;
    std::vector<UiText*> itemNameTexts;
    std::vector<UiText*> itemCountTexts;
    std::vector<UiVisual*> itemIcons;
    std::vector<UiVisual*> itemOutlines;

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
    float verticalPadding = 50.0f;

    bool isDragging = false;
    float dragOffset = 0.0f;

    glm::vec4 colorNormal = glm::vec4(0.2f, 0.2f, 0.2f, 0.8f);
    glm::vec4 colorSelected = glm::vec4(0.4f, 0.6f, 0.4f, 1.0f);

    ScrollingList() = default;

    void Initialize(Font* font) {
        listFont = font;
        Scene* mainScene = GetScene();
        SceneNode* listRoot = GetNode();
        
        this->layout = listRoot->AddObjectIfMissing<UiLayout>(
            glm::uvec2(500, 1000), glm::ivec2(-40, 0), 10, AnchorPoint::CenterRight);

        Texture2D* bgTexture = mainScene->Resources()->Get<Texture2D>(
            "./res/textures/ui/2d/equipment_list.png", Texture2D::ColorTextureRGBA
        );
        auto* listRootBg = listRoot->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f), bgTexture);
        listRootBg->SetEnabled(false);

        SceneNode* scrollbarTrackNode = mainScene->GetOrCreateNode(listRoot, "Scrollbar Track");
        scrollbarTrackNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(20, 900), glm::ivec2(-20, 0), 11, AnchorPoint::CenterRight);
        auto* trackVisual = scrollbarTrackNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));
        trackVisual->SetEnabled(false);
        scrollbarTrackNode->AddObjectIfMissing<WheelTag>();

        scrollbarHandleNode = mainScene->GetOrCreateNode(scrollbarTrackNode, "Scrollbar Handle");
        scrollbarHandleNode->AddObjectIfMissing<UiLayout>(
            glm::uvec2(20, handleHeight), glm::ivec2(0, 0), 12, AnchorPoint::Center);
        scrollbarHandleNode->AddObjectIfMissing<WheelTag>();
        
        auto* handleVisual = scrollbarHandleNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        handleVisual->colorHovered = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
        handleVisual->SetEnabled(false);
        
        scrollbarHandle = scrollbarHandleNode->AddObjectIfMissing<UiInteractable>();
    }

    void RefreshItems(const std::vector<ScrollingListItemData>& itemsData) {
        if (!listFont || !layout) return;
        
        Scene* mainScene = GetScene();
        SceneNode* listRoot = GetNode();
        float listWidth = 440.0f;

        activeItemCount = static_cast<int>(itemsData.size());

        for (size_t i = 0; i < itemsData.size(); i++) {
            if (i >= itemNodes.size()) {
                SceneNode* itemNode = mainScene->GetOrCreateNode(listRoot, "ListItem_" + std::to_string(i));
                itemNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(listWidth, itemHeight), glm::ivec2(0, 0), 11, AnchorPoint::Center);
                itemNode->AddObjectIfMissing<WheelTag>();
                
                auto* interactable = itemNode->AddObjectIfMissing<UiInteractable>();

                SceneNode* outlineNode = mainScene->GetOrCreateNode(itemNode, "IconOutline_" + std::to_string(i));
                outlineNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(itemHeight - 6, itemHeight - 6), glm::ivec2(5, -2), 11, AnchorPoint::CenterLeft);
                outlineNode->AddObjectIfMissing<WheelTag>();
                auto* outlineVisual = outlineNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                
                SceneNode* iconNode = mainScene->GetOrCreateNode(itemNode, "Icon_" + std::to_string(i));
                iconNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(itemHeight - 10, itemHeight - 10), glm::ivec2(5, 0), 12, AnchorPoint::CenterLeft);
                iconNode->AddObjectIfMissing<WheelTag>();
                auto* iconVis = iconNode->AddObjectIfMissing<UiVisual>(glm::vec4(1.0f));

                SceneNode* textContainerNode = mainScene->GetOrCreateNode(itemNode, "TextContainer_" + std::to_string(i));
                textContainerNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(listWidth - itemHeight - 20, itemHeight), glm::ivec2(itemHeight + 10, 0), 12, AnchorPoint::CenterLeft);

                SceneNode* textNode = mainScene->GetOrCreateNode(textContainerNode, "NameText_" + std::to_string(i));
                textNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(listWidth - itemHeight - 70, itemHeight), glm::ivec2(0, 0), 13, AnchorPoint::CenterLeft);
                textNode->AddObjectIfMissing<WheelTag>();
                auto* nameTxt = textNode->AddObjectIfMissing<UiText>("", listFont);
                nameTxt->fontSize = 24.0f;
                nameTxt->alignment = TextAlignment::Left;
                nameTxt->verticalAlignment = TextVerticalAlignment::Middle;

                SceneNode* countNode = mainScene->GetOrCreateNode(textContainerNode, "CountText_" + std::to_string(i));
                countNode->AddObjectIfMissing<UiLayout>(
                    glm::uvec2(50, itemHeight), glm::ivec2(-10, 0), 13, AnchorPoint::CenterRight);
                countNode->AddObjectIfMissing<WheelTag>();
                auto* countTxt = countNode->AddObjectIfMissing<UiText>("", listFont);
                countTxt->fontSize = 24.0f;
                countTxt->alignment = TextAlignment::Right;
                countTxt->verticalAlignment = TextVerticalAlignment::Middle;

                itemNodes.push_back(itemNode);
                itemInteractables.push_back(interactable);
                itemOutlines.push_back(outlineVisual);
                itemIcons.push_back(iconVis);
                itemNameTexts.push_back(nameTxt);
                itemCountTexts.push_back(countTxt);
            }

            itemNodes[i]->SetEnabled(true);
            itemNameTexts[i]->text = itemsData[i].text;
            itemCountTexts[i]->text = "x" + std::to_string(itemsData[i].count);
            
            if (itemsData[i].icon) {
                itemIcons[i]->texture = itemsData[i].icon;
                itemIcons[i]->color = glm::vec4(1.0f);

                itemOutlines[i]->texture = itemsData[i].icon;
                itemOutlines[i]->color = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
            } else {
                itemIcons[i]->texture = nullptr;
                itemIcons[i]->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

                itemOutlines[i]->texture = nullptr;
                itemOutlines[i]->color = glm::vec4(0.0f);
            }
        }

        for (size_t i = itemsData.size(); i < itemNodes.size(); i++) {
            itemNodes[i]->SetEnabled(false);
        }

        int maxIndex = activeItemCount - 1;
        if (maxIndex < 0) maxIndex = 0;
        selectedIndex = std::clamp(selectedIndex, 0, maxIndex);
    }

    void Update() {
        auto* input = GetScene()->GetComponent<InputSystem>();
        if (!input || !layout || activeItemCount == 0) return;

        int maxIndex = activeItemCount - 1;

        if (input->KeyDown(Key::Down) && selectedIndex < maxIndex) {
            selectedIndex++;
        }
        if (input->KeyDown(Key::Up) && selectedIndex > 0) {
            selectedIndex--;
        }

        selectedIndex = std::clamp(selectedIndex, 0, maxIndex);

        for (size_t i = 0; i < activeItemCount; i++) {
            if (itemInteractables[i]->isHovered) {
                selectedIndex = i;
            }
        }

        float rowHeight = itemHeight + spacing;
        float selectedTopEdge = selectedIndex * rowHeight;
        float selectedBottomEdge = selectedTopEdge + itemHeight;
        float visibleHeight = static_cast<float>(layout->size.y);
        float paddedVisibleHeight = visibleHeight - (verticalPadding * 2.0f);
        float totalHeight = activeItemCount * rowHeight - spacing;
        float maxScroll = std::max(0.0f, totalHeight - paddedVisibleHeight);

        if (selectedTopEdge < targetScrollY) {
            targetScrollY = selectedTopEdge;
        } else if (selectedBottomEdge > targetScrollY + paddedVisibleHeight) {
            targetScrollY = selectedBottomEdge - paddedVisibleHeight;
        }

        if (scrollbarHandle) {
            glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
            float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;
            float virtualMouseY = input->GetMousePosition().y / scaleFactor;
            float relativeMouseY = virtualMouseY - (UiLayoutSystem::VIRTUAL_RESOLUTION.y / 2.0f);

            if (scrollbarHandle->isDown) {
                isDragging = true;
                float currentHandleY = 0.0f;
                if (auto* handleLayout = scrollbarHandleNode->GetObject<UiLayout>()) {
                    currentHandleY = handleLayout->offset.y;
                }
                dragOffset = relativeMouseY - currentHandleY;
            }

            if (isDragging) {
                if (!scrollbarHandle->isPressed && !scrollbarHandle->isDown) {
                    isDragging = false;
                } else {
                    float targetHandleY = relativeMouseY - dragOffset;
                    float trackTop = -visibleHeight / 2.0f + verticalPadding + handleHeight / 2.0f;
                    float trackBot = visibleHeight / 2.0f - verticalPadding - handleHeight / 2.0f;
                    
                    float trackRange = trackBot - trackTop;
                    float fraction = 0.0f;
                    if (trackRange > 0.001f) {
                        fraction = (targetHandleY - trackTop) / trackRange;
                    }
                    fraction = std::clamp(fraction, 0.0f, 1.0f);
                    
                    targetScrollY = fraction * maxScroll;
                    scrollY = targetScrollY;
                }
            }
        }

        targetScrollY = std::clamp(targetScrollY, 0.0f, maxScroll);
        
        if (!isDragging) {
            scrollY += (targetScrollY - scrollY) * scrollSpeed * Time::UnscaledDelta();
        }

        float startY = -visibleHeight / 2.0f + verticalPadding + itemHeight / 2.0f;

        for (size_t i = 0; i < activeItemCount; i++) {
            SceneNode* childNode = itemNodes[i];
            UiLayout* childLayout = childNode->GetObject<UiLayout>();
            
            float targetY = startY + (i * rowHeight) - scrollY;
            childLayout->offset = glm::ivec2(0, static_cast<int>(targetY));

            if (auto* visual = childNode->GetObject<UiVisual>()) {
                visual->color = (i == selectedIndex) ? colorSelected : colorNormal;
            }
        }

        if (scrollbarHandleNode && maxScroll > 0.0f) {
            float handleFraction = maxScroll > 0.0f ? (scrollY / maxScroll) : 0.0f;

            float trackTop = -visibleHeight / 2.0f + verticalPadding + handleHeight / 2.0f;
            float trackBot = visibleHeight / 2.0f - verticalPadding - handleHeight / 2.0f;
            float handleRange = trackBot - trackTop;
            float handleY = trackTop + (handleFraction * handleRange); 
            
            if (auto* handleLayout = scrollbarHandleNode->GetObject<UiLayout>()) {
                handleLayout->offset = glm::ivec2(0, static_cast<int>(handleY));
            }
        }
    }
};
