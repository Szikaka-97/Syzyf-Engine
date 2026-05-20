#include "./include/game_scripts/enemies/AiSimplified.h"
#include"./include/game_scripts/enemies/EnemySkeleton.h"
#include <Player.h>
#include <Scene.h>
#include <./include/game_scripts/enemies/EnemyBase.h>

#include <glm/glm.hpp>


  void EnemySkeleton::Update() {

    EnsureBody();
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
        StopMoving();
        Attack();
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
