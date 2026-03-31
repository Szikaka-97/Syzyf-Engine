#pragma once

#include "GameObject.h"
#include "Scene.h"

#include <glm/glm.hpp>

#include <vector>

class SkeletonComponent : public GameObject {
public:
  std::vector<glm::mat4> inverseBindMatrices;
  std::vector<SceneNode*> joints;
  std::vector<glm::mat4> jointMatrices;
  SceneNode* skeletonRoot = nullptr;
  int bufferOffset = 0;
};
