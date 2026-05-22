#pragma once

#include "GameObjectSystem.h"
#include "animation/AnimationComponent.h"

class AnimationSystem : public GameObjectSystem<AnimationComponent> {
public:
  AnimationSystem(Scene* scene);
  virtual ~AnimationSystem() = default;

  virtual void OnPreUpdate();

  virtual void UnregisterObjectForced(GameObject* obj);
};
