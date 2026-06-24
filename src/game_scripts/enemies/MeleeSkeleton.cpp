#include "./include/game_scripts/enemies/AiSimplified.h"
#include"./include/game_scripts/enemies/MeleeSkeleton.h"
#include <Scene.h>
#include <./include/game_scripts/enemies/EnemyBase.h>

#include "game_scripts/PlayerController.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/spdlog.h>

void MeleeSkeleton::StartAttack() {
    m_IsAttacking = true;
    StopMoving();

    PlayAttackAnimation("attack.001");

    glm::vec3 diff = currentPos - m_TargetPosition;
    diff.y = 0.0f;
    float dist = glm::length(diff);
    if (dist <= attackRange) {
        auto* pc = m_TargetNode->GetObject<PlayerController>();
        if (pc) pc->TakeDamage(1);
    }

}

void MeleeSkeleton::UpdateAttackSequence() {

}

void MeleeSkeleton::Update() {
    EnsureBody();
    LockXZRotation();
    if (!m_TargetNode) return;

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    if (!myNode) return;

    if (m_Body) {
        m_Body->SetCollisionLayerAndMask({2}, ~(1u << 1u));
    }

    if (!m_AnimInitialized) {
        m_AnimInitialized = true;
        AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
        if (anim) {
            SetAttackAnimation(anim);
            spdlog::info("MeleeSkeleton: AnimationComponent found and initialized");
        } else {
            spdlog::error("MeleeSkeleton: AnimationComponent NOT FOUND in children!");
        }
    }

    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    // const float minDist = 1.0f;
    // glm::vec3 toPlayer = m_TargetPosition - currentPos;
    // toPlayer.y = 0.0f;
    // float flatDist = glm::length(toPlayer);
    // if (flatDist < minDist && flatDist > 0.01f) {
    //     StopMoving();
    //     glm::vec3 safePos = m_TargetPosition - glm::normalize(toPlayer) * minDist;
    //     safePos.y = currentPos.y;
    //     m_Body->SetPosition(safePos);
    //     myNode->GlobalTransform().Position() = safePos;
    //     currentPos = safePos;
    // }

    UpdateAttackAnimation();
    if (m_InAttackAnimation) { StopMoving(); return; }

    if (isPlayerInRoom) {
        // XZ distance — ignoruj różnicę Y między capsule
        glm::vec3 diff = currentPos - m_TargetPosition;
        diff.y = 0.0f;
        float dist = glm::length(diff);
        //spdlog::error(dist);
        //if      (m_hp <= 30)          currentState = States::FLEEING;
         if (dist <= attackRange) currentState = States::ATTACKING;
        else                          currentState = States::CHASING;
    } else {
        currentState = States::PATROLLING;
    }



    switch (currentState) {
    case States::ATTACKING: {
        StopMoving();
        glm::vec3 dir = m_TargetPosition - currentPos;
        dir.y = 0.0f;
        if (glm::length(dir) > 0.01f)
            RotateNode(glm::normalize(dir));
        SetLoopingAnimation("attack.001");
        StartAttack();
        Attack();
        break;
    }
    case States::CHASING: {
        // float dist = glm::distance(currentPos, m_TargetPosition);
        // if (dist > minDist + 0.5f) {  // zatrzymaj się z marginesem przed minDist
        //     DirectChase();
        //     SetLoopingAnimation("walk.001");
        // } else {
        //     StopMoving();
        //     glm::vec3 dir = m_TargetPosition - currentPos;
        //     dir.y = 0.0f;
        //     if (glm::length(dir) > 0.01f)
        //         RotateNode(glm::normalize(dir));
        //     SetLoopingAnimation("idle.001");
        // }
        // break;
        DirectChase();
        SetLoopingAnimation("walk.001");
        break;
    }
    case States::PATROLLING:
        Patrol();
        SetLoopingAnimation("walk.001");
        break;
    default:
        break;
    }
}
