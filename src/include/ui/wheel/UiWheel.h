#pragma once

#include "GameObject.h"
#include "GameObjectSystem.h"
#include "DepthOfField.h"
#include "TimeSystem.h"
#include "TweenSystem.h"
#include "Scene.h"
#include "InputSystem.h"
#include "ui/objects/UiInteractable.h"
#include "ui/objects/UiVisual.h"

class WheelTag : public GameObject {};

class WheelSystem : public GameObjectSystem<WheelTag> {
  private:
    const float BLUR_DURATION = 0.2f;

    InputSystem* inputSystem = nullptr;
    DepthOfField* dof = nullptr;
    TweenSystem* tweenSystem = nullptr;

    TweenHandle blurTween;

    TweenHandle unblurTween;

  public:
    WheelSystem(Scene* scene) : GameObjectSystem<WheelTag>(scene) {}

    void OnPreUpdate() {
        // Caching objects/systems
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
        if (!inputSystem || !tweenSystem || !dof)
            return;

        // Input
        if (inputSystem->KeyDown(Key::Tab)) {
            Time::SetTimeScale(0.1f);

            for (WheelTag* object : IterateObjects()) {
                if (auto* uiVisual = object->GetObject<UiVisual>()) {
                    uiVisual->SetEnabled(true);
                    uiVisual->color.a = 0.0f;
                }
                if (auto* uiInteractable = object->GetObject<UiInteractable>()) {
                    uiInteractable->isInteractable = true;
                }

                dof->SetEnabled(true);

                this->unblurTween.SetPlaying(false);
                float currentValue = dof->finalMixFactor;

                float diff = (1.0f - currentValue);
                float duration = diff = BLUR_DURATION * diff;

                TweenConfig blurConfig = {.initialValue = currentValue,
                                          .targetValue = 1.0f,
                                          .duration = duration};

                this->blurTween = std::move(
                    tweenSystem->CreateTween(blurConfig)
                        .Bind([this](float newValue) {
                            if (std::vector<DepthOfField*> dofObjects =
                                    this->GetScene()
                                        ->FindObjectsOfType<DepthOfField>();
                                !dofObjects.empty()) {
                                dofObjects[0]->finalMixFactor = newValue;
                            }
                            for (auto* object : this->IterateObjects()) {
                                if (auto* visual = object->GetObject<UiVisual>()) {
                                    // This wouldn't work if the ui didn't start with 1.0f alpha
                                    //  maybe add an override field or sth
                                    visual->color.a = newValue;
                                    if (visual->colorClicked.has_value()) visual->colorClicked->a = newValue;
                                    if (visual->colorDisabled.has_value()) visual->colorDisabled->a = newValue;
                                    if (visual->colorHovered.has_value()) visual->colorHovered->a = newValue;
                                }
                            }
                        }));
            }
        }

        if (inputSystem->KeyUp(Key::Tab)) {
            Time::SetTimeScale(1.0f);

            for (WheelTag* object : IterateObjects()) {
                this->blurTween.SetPlaying(false);

                float currentValue = this->dof->finalMixFactor;
                float duration = BLUR_DURATION * currentValue;

                TweenConfig unblurConfig = {.initialValue = currentValue,
                                            .targetValue = 0.0f,
                                            .duration = duration};

                this->unblurTween = std::move(
                    tweenSystem->CreateTween(unblurConfig)
                        .Bind([this](float newValue) {
                            if (std::vector<DepthOfField*> dofObjects =
                                    this->GetScene()
                                        ->FindObjectsOfType<DepthOfField>();
                                !dofObjects.empty()) {
                                dofObjects[0]->finalMixFactor = newValue;
                            }
                            for (auto* object : this->IterateObjects()) {
                                if (auto* visual = object->GetObject<UiVisual>()) {
                                    visual->color.a = newValue;
                                    if (visual->colorClicked.has_value()) visual->colorClicked->a = newValue;
                                    if (visual->colorDisabled.has_value()) visual->colorDisabled->a = newValue;
                                    if (visual->colorHovered.has_value()) visual->colorHovered->a = newValue;
                                }
                            }
                        })
                        .OnComplete([this]() {
                            if (std::vector<DepthOfField*> dofObjects =
                                    this->GetScene()
                                        ->FindObjectsOfType<DepthOfField>();
                                !dofObjects.empty()) {
                                dofObjects[0]->SetEnabled(false);
                            }
                            for (auto* object : this->IterateObjects()) {
                                if (auto* visual = object->GetObject<UiVisual>()) {
                                    visual->SetEnabled(false);
                                }
                                if (auto* interactable = object->GetObject<UiInteractable>()) {
                                    interactable->isInteractable = false;
                                }
                            }
                        }));
            }
        }
    }
};


