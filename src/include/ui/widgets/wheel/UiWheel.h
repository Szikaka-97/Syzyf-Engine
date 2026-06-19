#pragma once

#include "GameObject.h"
#include "GameObjectSystem.h"
#include "DepthOfField.h"
#include "TimeSystem.h"
#include "TweenSystem.h"
#include "Scene.h"
#include "InputSystem.h"
#include "ui/objects/UiInteractable.h"
#include "game_scripts/PotionInventory.h"
#include "ui/objects/UiVisual.h"
#include "ui/objects/UiText.h"
#include <unordered_map>

class WheelTag : public GameObject {};

// chuj
#include "game_scripts/ui/ScrollingList.h"

class WheelSystem : public GameObjectSystem<WheelTag> {
  private:
    std::unordered_map<GameObject*, float> baseAlphas;
    const float BLUR_DURATION = 0.2f;

    InputSystem* inputSystem = nullptr;
    DepthOfField* dof = nullptr;
    TweenSystem* tweenSystem = nullptr;

    TweenHandle blurTween;
    TweenHandle unblurTween;

  public:
    bool isMenuBlocking = false;
    bool isTabHeld = false;

    WheelSystem(Scene* scene) : GameObjectSystem<WheelTag>(scene) {}

    void OpenWheel() {
        Time::SetTimeScale(0.1f);

        for (WheelTag* object : IterateObjects()) {
            if (auto* uiVisual = object->GetObject<UiVisual>()) {
                if (this->baseAlphas.find(uiVisual) == this->baseAlphas.end()) {
                    this->baseAlphas[uiVisual] = uiVisual->color.a;
                }
                uiVisual->SetEnabled(true);
                uiVisual->color.a = 0.0f;
            }
            if (auto* uiText = object->GetObject<UiText>()) {
                if (this->baseAlphas.find(uiText) == this->baseAlphas.end()) {
                    this->baseAlphas[uiText] = uiText->color.a;
                }
                uiText->SetEnabled(true);
                uiText->color.a = 0.0f;
            }
            if (auto* uiInteractable = object->GetObject<UiInteractable>()) {
                uiInteractable->isInteractable = true;
            }
        }

        if (dof) dof->SetEnabled(true);

        if (tweenSystem->IsValid(this->unblurTween)) this->unblurTween.SetPlaying(false);
        
        float currentValue = dof ? dof->finalMixFactor : 0.0f;
        float diff = (1.0f - currentValue);
        float duration = BLUR_DURATION * diff;

        TweenConfig blurConfig = {.initialValue = currentValue,
                                  .targetValue = 1.0f,
                                  .duration = duration};

        this->blurTween = std::move(
            tweenSystem->CreateTween(blurConfig)
                .Bind([this](float newValue) {
                    if (this->dof) {
                        this->dof->finalMixFactor = newValue;
                    }
                    
                    for (auto* object : this->IterateObjects()) {
                        if (auto* visual = object->GetObject<UiVisual>()) {
                            float baseA = this->baseAlphas.count(visual) ? this->baseAlphas[visual] : 1.0f;
                            float finalAlpha = newValue * baseA;

                            visual->color.a = finalAlpha;
                            if (visual->colorClicked.has_value()) visual->colorClicked->a = finalAlpha;
                            if (visual->colorDisabled.has_value()) visual->colorDisabled->a = finalAlpha;
                            if (visual->colorHovered.has_value()) visual->colorHovered->a = finalAlpha;
                        }
                        if (auto* text = object->GetObject<UiText>()) {
                            float baseA = this->baseAlphas.count(text) ? this->baseAlphas[text] : 1.0f;
                            text->color.a = newValue * baseA;
                        }
                    }
                }));
    }

    void CloseWheel() {
        Time::SetTimeScale(1.0f);

        if (tweenSystem->IsValid(this->blurTween)) this->blurTween.SetPlaying(false);

        float currentValue = dof ? dof->finalMixFactor : 1.0f;
        float duration = BLUR_DURATION * currentValue;

        TweenConfig unblurConfig = {.initialValue = currentValue,
                                    .targetValue = 0.0f,
                                    .duration = duration};

        this->unblurTween = std::move(
            tweenSystem->CreateTween(unblurConfig)
                .Bind([this](float newValue) {
                    if (this->dof) {
                        this->dof->finalMixFactor = newValue;
                    }
                    
                    for (auto* object : this->IterateObjects()) {
                        if (auto* visual = object->GetObject<UiVisual>()) {
                            float baseA = this->baseAlphas.count(visual) ? this->baseAlphas[visual] : 1.0f;
                            float finalAlpha = newValue * baseA;

                            visual->color.a = finalAlpha;
                            if (visual->colorClicked.has_value()) visual->colorClicked->a = finalAlpha;
                            if (visual->colorDisabled.has_value()) visual->colorDisabled->a = finalAlpha;
                            if (visual->colorHovered.has_value()) visual->colorHovered->a = finalAlpha;
                        }
                        if (auto* text = object->GetObject<UiText>()) {
                            float baseA = this->baseAlphas.count(text) ? this->baseAlphas[text] : 1.0f;
                            text->color.a = newValue * baseA;
                        }
                    }
                })
                .OnComplete([this]() {
                    if (this->dof) {
                        this->dof->SetEnabled(false);
                    }
                    
                    for (auto* object : this->IterateObjects()) {
                        if (auto* visual = object->GetObject<UiVisual>()) {
                            visual->SetEnabled(false);
                        }
                        if (auto* text = object->GetObject<UiText>()) {
                            text->SetEnabled(false);
                        }
                        if (auto* interactable = object->GetObject<UiInteractable>()) {
                            interactable->isInteractable = false;
                        }
                    }
                }));
    }

    void OnPreUpdate() {
        if (this->inputSystem == nullptr) {
            this->inputSystem = this->GetScene()->inputSystem;
        }
        
        if (this->dof == nullptr) {
            std::vector<DepthOfField*> dofObjects =
                this->GetScene()->FindObjectsOfType<DepthOfField>();
            if (!dofObjects.empty()) {
                this->dof = dofObjects[0];
                this->dof->finalMixFactor = 0.0f;
            }
        }
        
        if (this->tweenSystem == nullptr) {
            this->tweenSystem = this->GetScene()->GetComponent<TweenSystem>();
        }
        
        if (!inputSystem || !tweenSystem) {
            return;
        }

        if (inputSystem->KeyPressed(Key::Tab)) {
            auto lists = GetScene()->FindObjectsOfType<ScrollingList>();
            if (!lists.empty()) {
            std::vector<ScrollingListItemData> itemsData;
            auto playerInventory = PotionInventory::GetOwnedIngredientDefinitions();

            for (const auto& entry : playerInventory) {
                int count = PotionInventory::GetIngredientCount(entry.inventoryKey);
                if (count > 0) {
                    std::string cleanName = entry.displayName;
                    std::size_t lastSpace = cleanName.find_last_of(' ');

                    if (lastSpace != std::string::npos) {
                        cleanName = cleanName.substr(0, lastSpace);
                    }

                    itemsData.push_back({ cleanName, count, nullptr });
                }
            }
            
            lists[0]->RefreshItems(itemsData); 
        }
        }

        if (inputSystem->KeyDown(Key::Tab)) isTabHeld = true;
        if (inputSystem->KeyUp(Key::Tab)) isTabHeld = false;

        if (inputSystem->KeyDown(Key::Tab) && !isMenuBlocking) {
            OpenWheel();
        }

        if (inputSystem->KeyUp(Key::Tab) && !isMenuBlocking) {
            CloseWheel();
        }
    }
};
