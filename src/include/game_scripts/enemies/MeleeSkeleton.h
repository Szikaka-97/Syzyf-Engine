#pragma once

#include "./include/game_scripts/enemies/AiSimplified.h"
//#include <Player.h>
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

  	SceneNode* enemyModel =
	   ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/szkielet4.glb")
	       ->Instantiate(GetScene(), GetNode(), "MeleeSkeletonModel");

  	enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
  	AnimationComponent* enemyAnim = GetNode()->GetObjectInChildren<AnimationComponent>();
  	SetAttackAnimation(enemyAnim);

};
	void StartAttack();
  void Update();
};


