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
    // m_TargetPosition = GetObject<Player>()->GlobalTransform().Position();
    if (m_TargetNode)
      m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    else
      return;
    if (!m_NavGrid) {
      auto grids = GetScene()->FindObjectsOfType<NavigationGrid>();
      if (!grids.empty()) m_NavGrid = grids[0];
    }

    if (!myNode) return;

    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
    glm::vec3 dirToTarget = m_TargetPosition - currentPos;

    UpdateAttackAnimation();
    if (m_InAttackAnimation) {
      StopMoving();
      return;
    }

    if (m_IsAttacking) {
        StopMoving();
        UpdateAttackSequence();
        return;
    }

    if (isPlayerInRoom) {
      float dist = glm::distance(currentPos, m_TargetPosition);
      if (m_hp <= 30) {
        currentState = States::FLEEING;
      } else if (dist <= attackRange) {
        currentState = States::ATTACKING;
      } else if (m_UsingAStar) {
        currentState = States::AVOIDING_OBSTACLE;
      } else {
        currentState = States::CHASING;
      }
    } else {
      currentState = States::PATROLLING;
    }

    if (currentState == States::CHASING) {
      UpdateStuckDetection();
    }

    if (currentState != States::AVOIDING_OBSTACLE) {
      m_UsingAStar = false;
      m_Path.clear();
    }

    switch (currentState) {
      case States::PATROLLING:
        m_StuckTimer = 0.0f;
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
      case States::AVOIDING_OBSTACLE:
        AstarChase();
        break;
    }

    DrawDebugView();
  }
