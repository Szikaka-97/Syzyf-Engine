#include "animation/SkeletonSystem.h"
#include "GameObjectSystem.h"
#include "animation/SkeletonComponent.h"

#include "Scene.h"

SkeletonSystem::SkeletonSystem(Scene* scene) : GameObjectSystem<SkeletonComponent>(scene) {
  glGenBuffers(1, &this->skinningBuffer);
  spdlog::info("Skeleton system added");
}

void SkeletonSystem::OnPreUpdate() {
  auto objects = GetScene()->FindObjectsOfType<SkeletonComponent>();
   
  std::size_t totalJoints = 0;
  for (auto* skeleton : objects) {
    totalJoints += skeleton->joints.size();
  }

  if (totalJoints == 0) {
    return;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->skinningBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, totalJoints * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

  int currentOffset = 0;
  for (auto* skeleton : objects) {
    skeleton->bufferOffset = currentOffset;
    skeleton->jointMatrices.resize(skeleton->joints.size());

    glm::mat4 inverseMeshGlobal = glm::inverse(skeleton->GetNode()->GlobalTransform().Value());

    for (std::size_t i = 0; i < skeleton->joints.size(); ++i) {
      SceneNode* jointNode = skeleton->joints[i];
      glm::mat4 jointGlobal = jointNode->GlobalTransform().Value();
      glm::mat4 ibm = skeleton->inverseBindMatrices[i];

      skeleton->jointMatrices[i] = inverseMeshGlobal * jointGlobal * ibm;
    }

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, currentOffset * sizeof(glm::mat4), skeleton->jointMatrices.size() * sizeof(glm::mat4), skeleton->jointMatrices.data());

    currentOffset += skeleton->joints.size();
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLuint SkeletonSystem::GetSkinningBufferHandle() {
  return this->skinningBuffer;
}
