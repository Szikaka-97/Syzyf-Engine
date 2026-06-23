#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"

#include <Scene.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

void EnemySkeleton::Update() {
    EnsureBody();

    if (!m_Body || !myNode || !m_TargetNode) {
        return;
    }

    if (GlobalTransform().Position().y < -10.0f) {
        TakeDamage(9999999);
        return;
    }

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
        float dist = glm::distance(currentPos, m_TargetPosition);

        if (m_hp <= 1) {
            currentState = States::FLEEING;
        }
        else if (dist <= attackRange) {
            currentState = States::ATTACKING;
        }
        else {
            currentState = States::CHASING;
        }
    }
    else {
        currentState = States::PATROLLING;
    }

    switch (currentState) {
    case States::ATTACKING:
        StopMoving();
        Attack();
        break;

    case States::CHASING:
    case States::FLEEING:
    case States::PATROLLING:
        UpdateMovementAnimation();
        break;

    default:
        StopMoving();
        SetAnimation("attacked.001");
        break;
    }
}

void EnemySkeleton::OnCollisionEnter() {}