#pragma once

#include "animation/SkeletonComponent.h"


class SkeletonSystem : public GameObjectSystem<SkeletonComponent> {
public:
  SkeletonSystem(Scene* scene);

  // preupdate or postupdate?
  void OnPreUpdate();
};
