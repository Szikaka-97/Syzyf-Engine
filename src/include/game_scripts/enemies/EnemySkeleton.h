#pragma once

#include "game_scripts/enemies/AiSimplified.h"
#include <Scene.h>
#include <game_scripts/enemies/EnemyBase.h>
#include <game_scripts/enemies/loot/LootPool.h>

#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
 public:
  EnemySkeleton() : EnemyBase() {
    SceneNode* enemyModel =
         ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/szkielet4.glb")
             ->Instantiate(GetScene(), GetNode(), "EnemySkeletonModel");

    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
    AnimationComponent* enemyAnim = GetNode()->GetObjectInChildren<AnimationComponent>();
    SetAttackAnimation(enemyAnim);
  };
  void Update();
//void DirectChaseWithFlock(const glm::vec3 & flockForce);
  void OnCollisionEnter();
  LootPool& GetLootPool() override {return LootPool::GetSkeletonLootPool();}
};


