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

    static SceneNode* Create(Scene& scene, Font* font, int layer = 0, const std::string& labelText = "", bool defaultState = false, SceneNode* parent = nullptr) {
        SceneNode* rootNode = scene.GetOrCreateNode(parent, "Checkbox Root");
        auto* layout = rootNode->AddObjectIfMissing<UiLayout>(glm::ivec2(200, 40), glm::ivec2(0, 0), layer, AnchorPoint::Center);

        auto* checkboxLogic = rootNode->AddObjectIfMissing<UiCheckbox>();
        checkboxLogic->isChecked = defaultState;

        SceneNode* backgroundNode = scene.GetOrCreateNode(rootNode, "Background");
        backgroundNode->AddObjectIfMissing<UiLayout>(glm::ivec2(30, 30), glm::ivec2(0, 0), layer + 1, AnchorPoint::CenterLeft);
        backgroundNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        auto* interactable = backgroundNode->AddObjectIfMissing<UiInteractable>();
        checkboxLogic->interactable = interactable;

        interactable->OnDown = [checkboxLogic]() {
            checkboxLogic->SetChecked(!checkboxLogic->isChecked);
        };

        SceneNode* markNode = scene.GetOrCreateNode(backgroundNode, "Checkmark");
        markNode->AddObjectIfMissing<UiLayout>(glm::ivec2(20, 20), glm::ivec2(0, 0), layer + 2, AnchorPoint::Center);
        auto* checkVisual = markNode->AddObjectIfMissing<UiVisual>(glm::vec4(0.0f, 0.8f, 0.2f, 1.0f));
        checkboxLogic->checkmarkVisual = checkVisual;

        SceneNode* textNode = scene.GetOrCreateNode(rootNode, "Label");
        textNode->AddObjectIfMissing<UiLayout>(glm::ivec2(150, 40), glm::ivec2(40, 0), layer + 2, AnchorPoint::CenterLeft);
        auto* text = textNode->AddObjectIfMissing<UiText>(labelText, font);
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
