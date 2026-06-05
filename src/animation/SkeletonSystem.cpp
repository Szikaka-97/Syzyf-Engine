#include "animation/SkeletonSystem.h"
#include "GameObjectSystem.h"
#include "animation/SkeletonComponent.h"

#include "Scene.h"

SkeletonSystem::SkeletonSystem(Scene* scene) : GameObjectSystem<SkeletonComponent>(scene) {
  glGenBuffers(1, &this->skinningBuffer);
  spdlog::info("Skeleton system added");
}

void SkeletonSystem::OnPreUpdate() {
  std::size_t totalJoints = 0;
  for (auto* skeleton : IterateObjects()) {
    totalJoints += skeleton->joints.size();
  }

  if (totalJoints == 0) {
    return;
  }

  if (batchedMatrices.size() < totalJoints) {
      batchedMatrices.resize(totalJoints);
  }

  int currentOffset = 0;
  for (auto* skeleton : IterateObjects()) {
      skeleton->bufferOffset = currentOffset;

      if (skeleton->jointMatrices.size() != skeleton->joints.size()) {
          skeleton->jointMatrices.resize(skeleton->joints.size());
      }

      glm::mat4 inverseMeshGlobal = glm::inverse(skeleton->GetNode()->GlobalTransform().Value());

      for (std::size_t i = 0; i < skeleton->joints.size(); ++i) {
          SceneNode* jointNode = skeleton->joints[i];
          glm::mat4 jointGlobal = jointNode->GlobalTransform().Value();
          glm::mat4 ibm = skeleton->inverseBindMatrices[i];

          glm::mat4 finalMatrix = inverseMeshGlobal * jointGlobal * ibm;

          skeleton->jointMatrices[i] = finalMatrix;

          batchedMatrices[currentOffset + i] = finalMatrix;
      }

      currentOffset += skeleton->joints.size();
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->skinningBuffer);

  std::size_t requiredSize = totalJoints * sizeof(glm::mat4);
  if (this->currentBufferSize < requiredSize) {
      glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
      this->currentBufferSize = requiredSize;
  }

  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, requiredSize, batchedMatrices.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLuint SkeletonSystem::GetSkinningBufferHandle() {
  return this->skinningBuffer;
}
