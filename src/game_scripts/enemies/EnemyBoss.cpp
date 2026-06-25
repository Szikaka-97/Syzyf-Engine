#include "game_scripts/PlayerController.h"


#include <./include/game_scripts/enemies/EnemyBoss.h>
#include <TimeSystem.h>

void EnemyBoss::StartAttack() {
    m_IsAttacking = true;
    StopMoving();
    if (m_Body)
        m_Body->SetLinearVelocity(glm::vec3(0.0f, m_Body->GetLinearVelocity().y, 0.0f));

    m_InAttackAnimation      = true;
    m_AttackAnimationElapsed = 0.0f;
    m_CurrentAnimation       = "attack.001";   // guard, identycznie jak EnemyBase::Attack()

    SetAnimation("attack.001");                // zatrzymuje walk, startuje attack
    m_normalAttackCounter++;

    glm::vec3 diff = currentPos - m_TargetPosition;
    diff.y = 0.0f;
    float dist = glm::length(diff);
    if (dist <= attackRange) {
        auto* pc = m_TargetNode->GetObject<PlayerController>();
        if (pc) pc->TakeDamage(5);
    }
}

void EnemyBoss::StartSpecialAttack() {
    m_isSpecialAttacking  = true;
    m_specialAttackPhase  = 1;
    m_normalAttackCounter = 0;
    StopMoving();
    if (m_Body)
        m_Body->SetLinearVelocity(glm::vec3(0.0f, m_Body->GetLinearVelocity().y, 0.0f));

    m_InAttackAnimation      = true;
    m_AttackAnimationElapsed = 0.0f;
    m_CurrentAnimation       = "special_start.001";

    SetAnimation("special_start.001");

    glm::vec3 diff = currentPos - m_TargetPosition;
    diff.y = 0.0f;
    float dist = glm::length(diff);
    if (dist <= attackRange) {
        auto* pc = m_TargetNode->GetObject<PlayerController>();
        if (pc) pc->TakeDamage(10);
    }
}

void EnemyBoss::Update() {
    //if (!myNode || !m_Body || m_hp <= 0) return;
    EnsureBody();
    if (!m_Body) { spdlog::error("EnemyBoss: body is null"); return; }

    LockXZRotation();
    if (!m_TargetNode || !myNode) return;

    if (!m_AnimInitialized) {
        m_AnimInitialized = true;
        AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
        if (anim) {
            SetAttackAnimation(anim);
            spdlog::info("EnemyBoss: AnimationComponent found and set");
        } else {
            spdlog::error("EnemyBoss: AnimationComponent NOT FOUND in children");
        }
    }

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    currentPos       = m_Body->GetPosition();

    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateAttackAnimation();
    UpdateStatusEffects();

    if (m_isSpecialAttacking && !m_InAttackAnimation) {
        if (m_specialAttackPhase == 1) {
            m_specialAttackPhase     = 2;
            m_InAttackAnimation      = true;
            m_AttackAnimationElapsed = 0.0f;
            PlayAttackAnimation("special_spin.001");
        } else if (m_specialAttackPhase == 2) {
            m_specialAttackPhase     = 3;
            m_InAttackAnimation      = true;
            m_AttackAnimationElapsed = 0.0f;
            PlayAttackAnimation("special_end.001");
        } else if (m_specialAttackPhase == 3) {
            m_isSpecialAttacking = false;
            m_IsAttacking        = false;
            m_specialAttackPhase = 0;
        }
    }

    if (m_InAttackAnimation || m_isSpecialAttacking) {
        StopMoving();
        if (m_Body)
            m_Body->SetLinearVelocity(glm::vec3(0.0f, m_Body->GetLinearVelocity().y, 0.0f));

        glm::vec3 toTarget = m_TargetPosition - currentPos;
        toTarget.y = 0.0f;
        if (glm::length(toTarget) > 0.1f) {
            toTarget = glm::normalize(toTarget);
            float targetYaw = atan2(toTarget.x, toTarget.z);
            glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
            glm::quat currentRot = myNode->GlobalTransform().Rotation();
            glm::quat newRot = glm::slerp(currentRot, targetRot, m_BossRotationSpeed * Time::Delta());
            m_Body->SetRotation(newRot);
            myNode->GlobalTransform().Rotation() = newRot;
        }
        return;
    }

    m_IsAttacking = false;

    if (IsConfused()) {
        currentState = States::PATROLLING;
    }
    else if (isPlayerInRoom) {
        glm::vec3 diff = currentPos - m_TargetPosition;
        diff.y = 0.0f;
        float dist = glm::length(diff);
        spdlog::error(dist);
        if (dist <= attackRange) {
            currentState = States::ATTACKING;
        } else {
            currentState = States::CHASING;
        }
    }
    else {
        currentState = States::PATROLLING;
    }

    switch (currentState) {
    case States::PATROLLING:
        Patrol();
        SetLoopingAnimation("walk.001");
        break;
    case States::CHASING:
        DirectChaseNoBoundary();
        SetLoopingAnimation("walk.001");
        break;
    case States::ATTACKING: {
        StopMoving();
        glm::vec3 dir = m_TargetPosition - currentPos;
        dir.y = 0.0f;
        if (glm::length(dir) > 0.01f)
            RotateNode(glm::normalize(dir));
        SetLoopingAnimation("attack.001");
        if (m_normalAttackCounter >= m_maxNormalAttackInterval)
            StartSpecialAttack();
        else
            StartAttack();
        break;
    }
    default:
        break;
    }
}