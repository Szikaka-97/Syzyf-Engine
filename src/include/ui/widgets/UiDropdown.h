#pragma once

#include "Scene.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"
class UiDropdown : public GameObject {
public:
    std::vector<std::string> options;
    int selectedIndex = 0;
    bool isOpen = false;

    std::function<void(int)> OnValueChanged = nullptr;

    UiText* mainText = nullptr;
    UiLayout* listLayout = nullptr;

    struct DropdownItem {
        UiVisual* visual;
        UiText* text;
        UiInteractable* interactable;
    };
    std::vector<DropdownItem> itemComponents;

    static SceneNode* Create(Scene& scene, Font* font, const std::vector<std::string>& optionStrings, int defaultIndex = 0, SceneNode* parent = nullptr) {
        SceneNode* rootNode = scene.CreateNode(parent, "Dropdown Root");
        auto* layout = rootNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), 0, AnchorPoint::Center);

        auto* dropdownLogic = rootNode->AddObject<UiDropdown>();
        dropdownLogic->options = optionStrings;
        dropdownLogic->selectedIndex = defaultIndex;

        // Main button
        SceneNode* mainButtonNode = scene.CreateNode(rootNode, "Main Button");
        mainButtonNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), 1, AnchorPoint::Center);
        auto* mainVisual = mainButtonNode->AddObject<UiVisual>(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        mainVisual->colorHovered = glm::vec4(0.5f, 0.5f , 0.5f, 1.0f);
        auto* mainInteractable = mainButtonNode->AddObject<UiInteractable>();

        SceneNode* mainTextNode = scene.CreateNode(mainButtonNode, "Main Text");
        mainTextNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), 2, AnchorPoint::Center);
        dropdownLogic->mainText = mainTextNode->AddObject<UiText>(optionStrings.empty() ? "" : optionStrings[defaultIndex], font);
        dropdownLogic->mainText->fontSize = 20.0f;

        // Dropdown list
        SceneNode* listNode = scene.CreateNode(rootNode, "List Node");
        dropdownLogic->listLayout = listNode->AddObject<UiLayout>(glm::ivec2(200, 40 * optionStrings.size()), glm::ivec2(0, 40), 10, AnchorPoint::TopCenter);

        for (size_t i = 0; i < optionStrings.size(); ++i) {
            SceneNode* itemNode = scene.CreateNode(listNode, "Item " + std::to_string(i));
            itemNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, i * 40), 11, AnchorPoint::TopCenter);

            auto* itemVisual = itemNode->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
            itemVisual->colorHovered = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
            auto* itemInteractable = itemNode->AddObject<UiInteractable>();

            SceneNode* itemTextNode = scene.CreateNode(itemNode, "Item Text");
            itemTextNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), 12, AnchorPoint::Center);
            auto* itemText = itemTextNode->AddObject<UiText>(optionStrings[i], font);
            itemText->fontSize = 20.0f;

            dropdownLogic->itemComponents.push_back({itemVisual, itemText, itemInteractable});

            itemInteractable->OnDown = [dropdownLogic, i]() {
                dropdownLogic->SelectIndex(static_cast<int>(i));
                dropdownLogic->SetOpen(false);
            };
        }

        mainInteractable->OnDown = [dropdownLogic]() {
            dropdownLogic->SetOpen(!dropdownLogic->isOpen);
        };

        dropdownLogic->SetOpen(false);

        return rootNode;
    }

    void SelectIndex(int index) {
        if (index >= 0 && index < options.size()) {
            selectedIndex = index;
            if (mainText) {
                mainText->text = options[index];
            }
            if (OnValueChanged) {
                OnValueChanged(selectedIndex);
            }
        }
    }

    void SetOpen(bool state) {
        isOpen = state;

        if (listLayout) {
            listLayout->offset.y = state ? 40 : 9999;
        }

        for (auto& component : itemComponents) {
            if (component.interactable) {
                component.interactable->isInteractable = state;
            }
            if (component.visual) {
                component.visual->SetEnabled(state);
            }
            if (component.text) {
                component.text->SetEnabled(state);
            }
        }
    }
};
