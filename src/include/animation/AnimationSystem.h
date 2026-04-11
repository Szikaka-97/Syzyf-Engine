#pragma once

#include "SceneComponent.h"
#include "animation/AnimationComponent.h"

class AnimationSystem : public GameObjectSystem<AnimationComponent> {
public:
  AnimationSystem(Scene* scene);
  virtual ~AnimationSystem() = default;

  virtual void OnPreUpdate();
};
