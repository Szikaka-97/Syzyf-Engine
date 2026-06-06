#include "./include/game_scripts/enemies/AiSimplified.h"
#include"./include/game_scripts/enemies/MeleeSkeleton.h"
#include <Scene.h>
#include <./include/game_scripts/enemies/EnemyBase.h>

#include "game_scripts/PlayerController.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

void MeleeSkeleton::StartAttack() {
    m_IsAttacking         = true;
    StopMoving();
    }

void MeleeSkeleton::UpdateAttackSequence() {

}

  void MeleeSkeleton::Update() {

    EnsureBody();
    LockXZRotation();
    if (!m_TargetNode) return;
 
    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    if (!myNode) return;
 
    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
 
    UpdateAttackAnimation();
    if (m_InAttackAnimation) { StopMoving(); return; }
 
    if (isPlayerInRoom) {
        float dist = glm::distance(currentPos, m_TargetPosition);
        if      (m_hp <= 30)          currentState = States::FLEEING;
        else if (dist <= attackRange) currentState = States::ATTACKING;
        else                          currentState = States::CHASING;
    } else {
        currentState = States::PATROLLING;
    }
    switch (currentState) {
      case States::PATROLLING:
        Patrol();
        break;
      case States::CHASING:
        DirectChase();
        break;
      case States::ATTACKING:
        if (!m_IsAttacking && m_AttackCooldown <= 0.0f)
                StartAttack();
            else {
                StopMoving();
                m_AttackCooldown -= Time::Delta();
            }
        break;
      case States::FLEEING:
        Flee();
        Attack();
        break;
    }

    //DrawDebugView();
  }
