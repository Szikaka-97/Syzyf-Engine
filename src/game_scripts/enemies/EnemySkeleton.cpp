#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"
#include <Scene.h>
#include <glm/glm.hpp>

void EnemySkeleton::Update() {
    EnsureBody();
    //if (!m_TargetNode || !myNode) return;

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    currentPos       = m_Body->GetPosition();

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
        case States::PATROLLING: {
                glm::vec3 patrolTarget = m_FlockingSystem->GetPatrolTarget(this);
                m_WalkPoint    = patrolTarget;
                m_WalkPointSet = true;

                glm::vec3 dir = patrolTarget - currentPos;
                dir.y = 0.0f;
                float dist = glm::length(dir);
                if (dist < 0.6f) {
                    m_FlockingSystem->RefreshPatrolTarget(this);
                    StopMoving();
                } else {
                   
                    glm::vec3 combined   = glm::normalize(dir) +
                        glm::clamp(flockForce, glm::vec3(-0.5f), glm::vec3(0.5f));
                    combined.y = 0.0f;
                    float len = glm::length(combined);
                    if (len > 0.001f) combined /= len;
                    MoveInDirection(combined);
                }
            
            break;
        }
        case States::CHASING: {
            
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

}

void EnemySkeleton::DirectChaseWithFlock(const glm::vec3& flockForce) {
    ChaseWithSteering(flockForce);
}

void EnemySkeleton::OnCollisionEnter() {}