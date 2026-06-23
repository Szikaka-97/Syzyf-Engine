#pragma once

#include "./include/game_scripts/enemies/AiSimplified.h"
#include "./include/game_scripts/enemies/EnemyBase.h"

class EnemyBoss : public EnemyBase{
public:
  bool m_IsAttacking = false;
  bool m_isSpecialAttacking = false;
  int m_specialAttackPhase = 0; // 0: brak, 1: start, 2: spin, 3: end

  int m_normalAttackCounter = 0;
  int m_maxNormalAttackInterval = 3;

  LootPool& GetLootPool() override{return LootPool::GetEnemyBossLootPool();}
  EnemyBoss() : EnemyBase() {
    this->attackRange = 4.0f;
     SceneNode* enemyModel =
         ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/krol_ciernia.glb")
             ->Instantiate(GetScene(), GetNode(), "EnemyBossModel");
     //enemyModel->SetParent(enemy1);
     //enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
     enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();


  };
  void StartAttack();
  void StartSpecialAttack();
  void Update();
};

