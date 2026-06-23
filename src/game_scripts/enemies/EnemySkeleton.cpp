#include "TimeSystem.h"
#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"
#include <Scene.h>
#include <glm/glm.hpp>

void EnemySkeleton::Update() {
    EnsureBody();
    if (!m_Body || !myNode || !m_TargetNode) return;

    if (GlobalTransform().Position().y < -10)
        TakeDamage(9999999);

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    currentPos       = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateStatusEffects();
    UpdateAttackAnimation();

    if (m_InAttackAnimation) {
        StopMoving();
        return;
    }

    if (isPlayerInRoom) {
        glm::vec3 toTarget = m_TargetPosition - currentPos;
        toTarget.y = 0.0f;
        float dist = glm::length(toTarget);

        if      (dist <= attackRange) currentState = States::ATTACKING;
        else                          currentState = States::CHASING;
    } else {
        currentState = States::PATROLLING;
    }

    switch (currentState) {
    case States::ATTACKING:
        StopMoving();
        Attack();
        break;
    case States::CHASING:
        SetLoopingAnimation("idle.001");
        break;
    case States::PATROLLING:
        SetLoopingAnimation("idle.001");
        Patrol();
        break;
    default:
        break;
    }
}

void EnemySkeleton::OnCollisionEnter() {}