#pragma once

#include "game_scripts/enemies/AiSimplified.h"
#include <Scene.h>
#include <game_scripts/enemies/EnemyBase.h>
#include <game_scripts/enemies/loot/LootPool.h>

#include <glm/glm.hpp>

class EnemySkeleton : public EnemyBase {
public:
  EnemySkeleton() : EnemyBase() { }

  void Awake();
  void Update();
  //void DirectChaseWithFlock(const glm::vec3 & flockForce);
  void OnCollisionEnter();
  LootPool& GetLootPool() override {return LootPool::GetSkeletonLootPool();}
};