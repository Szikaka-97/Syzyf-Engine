#include "animation/SkeletonSystem.h"
#include "GameObjectSystem.h"
#include "animation/SkeletonComponent.h"

#include "Scene.h"

SkeletonSystem::SkeletonSystem(Scene* scene) : GameObjectSystem<SkeletonComponent>(scene) {
  spdlog::info("Skeleton system added");
}

void SkeletonSystem::OnPreUpdate() {
  auto objects = GetScene()->FindObjectsOfType<SkeletonComponent>();
    
  for (auto* skeleton : objects) {
    SceneNode* meshNode = skeleton->GetNode(); 
        
    skeleton->jointMatrices.resize(skeleton->joints.size());

    glm::mat4 inverseMeshGlobal = glm::inverse(meshNode->GlobalTransform().Value());

    for (std::size_t i = 0; i < skeleton->joints.size(); ++i) {
      SceneNode* jointNode = skeleton->joints[i];
            
      glm::mat4 jointGlobal = jointNode->GlobalTransform().Value();
      glm::mat4 ibm = skeleton->inverseBindMatrices[i];

      skeleton->jointMatrices[i] = inverseMeshGlobal * jointGlobal * ibm;
    }
  }
}
