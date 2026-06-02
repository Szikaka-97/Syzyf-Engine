#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"
#include <Scene.h>
#include "Surface.h" 
#include <glm/glm.hpp>

void EnemySkeleton::Update() {
    EnsureBody();
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
        case States::CHASING: {
            glm::vec3 flockForce = m_FlockingSystem
                ? m_FlockingSystem->GetFlockingForce(this)
                : glm::vec3(0.0f);
            DirectChaseWithFlock(flockForce);
            break;
        }
        case States::ATTACKING:
            StopMoving();
            Attack();
            break;
        case States::FLEEING:
            Flee();
            Attack();
            break;
    }

    UpdateStatusEffects();

#ifndef NDEBUG
    DrawDebugView();
#endif
}

void EnemySkeleton::DirectChaseWithFlock(const glm::vec3& flockForce) {
    ChaseWithSteering(flockForce);
}

void EnemySkeleton::OnCollisionEnter() {}