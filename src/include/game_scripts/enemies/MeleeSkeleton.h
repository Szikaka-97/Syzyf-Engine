#pragma once

#include "./include/game_scripts/enemies/AiSimplified.h"
#include <Player.h>
#include <Scene.h>
#include <./include/game_scripts/enemies/EnemyBase.h>

#include "EnemySword.h"

#include <glm/glm.hpp>

class MeleeSkeleton : public EnemyBase {
 public:
	 bool m_IsAttacking = false;
	 void UpdateAttackSequence();
	 EnemySword* sword = nullptr;

MeleeSkeleton() : EnemyBase() {
	this->attackRange = 1.0f;

	auto* hand = GetNode()->FindNode("Cube.015");

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
