#pragma once

#include <Scene.h>
#include <game_scripts/enemies/EnemyBase.h>

#include <game_scripts/enemies/loot/LootPool.h>
#include "EnemySword.h"

#include <glm/glm.hpp>

class MeleeSkeleton : public EnemyBase {
 public:
	 bool m_IsAttacking = false;
	 void UpdateAttackSequence();
	 EnemySword* sword = nullptr;
	 
  LootPool& GetLootPool() override{return LootPool::GetMeleeSkeletonLootPool();}
MeleeSkeleton() : EnemyBase() {
	this->attackRange = 1.0f;

	auto* hand = GetNode()->FindNode("EnemyModel/rig.001_deform/Cube.015");

	if (hand) {
		sword = hand->AddObject<EnemySword>();
	}
	else {
		spdlog::warn("MeleeSkeleton: could not find hand node for attack animation");
	}

};
	void StartAttack();
  void Update();
};


