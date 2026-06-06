#pragma once

#include "GameObject.h"
#include "Scene.h"

#include <glm/glm.hpp>

#include <vector>

class SkeletonComponent : public GameObject {
public:
  SkeletonComponent() = default;

  serialized std::vector<glm::mat4> inverseBindMatrices;
  serialized std::vector<SceneNode*> joints;
  serialized std::vector<glm::mat4> jointMatrices;
  serialized SceneNode* skeletonRoot = nullptr;
  serialized int bufferOffset = 0;
};
