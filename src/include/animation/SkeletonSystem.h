#pragma once

#include "animation/SkeletonComponent.h"

#include <glad/glad.h>

class SkeletonSystem : public GameObjectSystem<SkeletonComponent> {
private:
  GLuint skinningBuffer;
public:
  SkeletonSystem(Scene* scene);

  // preupdate or postupdate?
  void OnPreUpdate();

  GLuint GetSkinningBufferHandle();
};
