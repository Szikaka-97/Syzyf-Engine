#pragma once

#include "GameObjectSystem.h"
#include "TweenSystem.h"
#include <unordered_map>

class InputSystem;
class DepthOfField;

class WheelTag : public GameObject {};

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

    void OpenWheel();

    void CloseWheel();

    virtual void OnPreUpdate() override;
};
