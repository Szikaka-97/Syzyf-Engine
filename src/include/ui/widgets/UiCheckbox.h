#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiLayout.h"
#include "ui/objects/UiText.h"
#include "ui/objects/UiVisual.h"

class UiCheckbox : public GameObject, public ImGuiDrawable {
public:
    bool isChecked = false;
    std::function<void(bool)> OnValueChanged = nullptr;

    UiInteractable* interactable = nullptr;
    UiVisual* checkmarkVisual = nullptr;

    static SceneNode* Create(Scene& scene, const std::string& labelText, Font* font, bool defaultState) {
        SceneNode* rootNode = scene.CreateNode("Checkbox Root");
        auto* layout = rootNode->AddObject<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), 0, AnchorPoint::Center);

        auto* checkboxLogic = rootNode->AddObject<UiCheckbox>();
        checkboxLogic->isChecked = defaultState;

        SceneNode* backgroundNode = scene.CreateNode(rootNode, "Background");
        backgroundNode->AddObject<UiLayout>(glm::ivec2(30, 30), glm::ivec2(0, 0), 0, AnchorPoint::CenterLeft);
        backgroundNode->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        auto* interactable = backgroundNode->AddObject<UiInteractable>();
        checkboxLogic->interactable = interactable;

        interactable->OnDown = [checkboxLogic]() {
            checkboxLogic->SetChecked(!checkboxLogic->isChecked);
        };

        SceneNode* markNode = scene.CreateNode(backgroundNode, "Checkmark");
        markNode->AddObject<UiLayout>(glm::ivec2(20, 20), glm::ivec2(0, 0), 1, AnchorPoint::Center);
        auto* checkVisual = markNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.8f, 0.2f, 1.0f));
        checkboxLogic->checkmarkVisual = checkVisual;

        SceneNode* textNode = scene.CreateNode(rootNode, "Label");
        textNode->AddObject<UiLayout>(glm::ivec2(150, 40), glm::ivec2(40, 0), 20, AnchorPoint::CenterLeft);
        auto* text = textNode->AddObject<UiText>(labelText, font);
        text->fontSize = 24.0f;
        text->color = glm::vec4(1.0f);

        checkboxLogic->SetChecked(defaultState);

        return rootNode;
    }

    void SetChecked(bool state) {
        this->isChecked = state;

        if (this->checkmarkVisual != nullptr) {
            this->checkmarkVisual->color.a = state ? 1.0f : 0.0f;
        }

        if (this->OnValueChanged != nullptr) {
            this->OnValueChanged(this->isChecked);
        }
    };

    void DrawImGui() {};
};
