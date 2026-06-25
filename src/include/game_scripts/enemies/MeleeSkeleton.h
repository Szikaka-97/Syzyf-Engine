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
  	this->m_Speed = 2.0f;

  	SceneNode* enemyModel =
	   ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/szkielet4.glb")
	       ->Instantiate(GetScene(), GetNode(), "MeleeSkeletonModel");

  	enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
  	AnimationComponent* enemyAnim = GetNode()->GetObjectInChildren<AnimationComponent>();
  	SetAttackAnimation(enemyAnim);

  	// SceneNode* sword = enemyModel->FindNode("rig_deform/DEF-upper_arm.L/DEF-upper_arm.L.001/DEF-forearm.L/DEF-forearm.L.001/DEF-hand.L/Plane");
  	// if (sword) {
  	// 	//spdlog::error("sword init2");
  	// 	sword->AddObject<EnemySword>();
  	// }
  	//spdlog::error("sword init3");

};
	void StartAttack();
  void Update();
};


