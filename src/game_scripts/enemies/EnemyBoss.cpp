#include <./include/game_scripts/enemies/EnemyBoss.h>
#include <TimeSystem.h>

void EnemyBoss::StartAttack() {
    m_IsAttacking = true;
    StopMoving();
    if (m_Body)
        m_Body->SetLinearVelocity(glm::vec3(0.0f, m_Body->GetLinearVelocity().y, 0.0f));

    m_InAttackAnimation      = true;
    m_AttackAnimationElapsed = 0.0f;

    PlayAttackAnimation("attack.001");
    m_normalAttackCounter++;
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

    PlayAttackAnimation("special_start.001");
}

void EnemyBoss::Update() {
    EnsureBody();
    if (!m_Body) { spdlog::error("EnemyBoss: body is null"); return; }

    LockXZRotation();
    if (!m_TargetNode || !myNode) return;

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    currentPos       = m_Body->GetPosition();

    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateAttackAnimation();

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

    if (isPlayerInRoom) {
        glm::vec3 toTarget = m_TargetPosition - currentPos;
        toTarget.y = 0.0f;
        float dist = glm::length(toTarget);

        if (currentState == States::ATTACKING)
            currentState = (dist <= attackRange * 1.3f) ? States::ATTACKING : States::CHASING;
        else
            currentState = (dist <= attackRange) ? States::ATTACKING : States::CHASING;
    } else {
        currentState = States::PATROLLING;
    }

    switch (currentState) {
    case States::PATROLLING:
        Patrol();
        break;
    case States::CHASING:
            DirectChaseNoBoundary();
        break;
    case States::ATTACKING:
        if (m_normalAttackCounter >= m_maxNormalAttackInterval)
            StartSpecialAttack();
        else
            StartAttack();
        return;
    }

    if (currentState != m_PreviousState) {
        if (currentState == States::CHASING || currentState == States::PATROLLING)
            SetLoopingAnimation("walk.001");   // ← loop
        m_PreviousState = currentState;
    }

}
