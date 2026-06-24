#include "TimeSystem.h"
#include <game_scripts/enemies/EnemySkeleton.h>
#include "game_scripts/enemies/FlockingSystem.h"
#include <Scene.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h> // Potrzebne do logowania błędów/sukcesu szukania komponentu

void EnemySkeleton::Update() {
    EnsureBody();
    if (!m_Body || !myNode || !m_TargetNode) return;
    LockXZRotation();
    if (!m_AnimInitialized) {
        m_AnimInitialized = true;
        AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
        if (anim) {
            SetAttackAnimation(anim);
            spdlog::info("EnemySkeleton: AnimationComponent found and initialized");
        } else {
            spdlog::error("EnemySkeleton: AnimationComponent NOT FOUND in children!");
        }
    }

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
        {
            glm::vec3 toPlayer = m_TargetPosition - currentPos;
            toPlayer.y = 0.0f;
            if (glm::length(toPlayer) > 0.01f)
                RotateNode(glm::normalize(toPlayer));
        }
        SetLoopingAnimation("idle.001");
        Attack();
        break;

    case States::CHASING:
        DirectChase();
        SetLoopingAnimation("walk.001");
        break;

    case States::PATROLLING:
        Patrol();
        SetLoopingAnimation("walk.001");
        break;

    default:
        break;
    }
}

void EnemySkeleton::OnCollisionEnter() {}