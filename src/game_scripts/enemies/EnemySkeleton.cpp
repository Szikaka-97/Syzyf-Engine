#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"
#include <Scene.h>
#include <glm/glm.hpp>

void EnemySkeleton::Update() {
    //spdlog::error("update");
    EnsureBody();
    if (!m_Body || !myNode || !m_TargetNode) return;

    if (GlobalTransform().Position().y < -10) {
        TakeDamage(9999999);
    }

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    currentPos       = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateStatusEffects();
    UpdateAttackAnimation();

    if (m_InAttackAnimation) {
        //spdlog::error("anim");
        StopMoving();
        return;
    }

    if (isPlayerInRoom) {
        //spdlog::error("player");
        float dist = glm::distance(currentPos, m_TargetPosition);
        if      (m_hp <= 1)          currentState = States::FLEEING;
        else if (dist <= attackRange) currentState = States::ATTACKING;
        else                          currentState = States::CHASING;
    } else {
        currentState = States::PATROLLING;
    }

    if (currentState == States::ATTACKING) {
        StopMoving();
        Attack();
    }
    //spdlog::error("end");
}

// void EnemySkeleton::DirectChaseWithFlock(const glm::vec3& flockForce) {
//     ChaseWithSteering(flockForce);
// }

void EnemySkeleton::OnCollisionEnter() {}
