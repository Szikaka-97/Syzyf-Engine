#include "./include/game_scripts/enemies/EnemyPotato.h"
#include "game_scripts/PlayerController.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

EnemyPotato::EnemyPotato() {
    attackRange = 3.0f;

    SceneNode* enemyModel =
         ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/ziemniak_remake4.glb")
             ->Instantiate(GetScene(), GetNode(), "EnemyPotatoModel");

    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
    AnimationComponent* enemyAnim = GetNode()->GetObjectInChildren<AnimationComponent>();
    SetAttackAnimation(enemyAnim);
}

void EnemyPotato::SpawnShadow() {
    if (m_ShadowNode) return;

    m_ShadowNode = ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/potato_shadow.glb")
         ->Instantiate(GetScene(), GetScene()->GetRootNode(), "PotatoShadow");

    m_ShadowNode->GlobalTransform().Position() =
        glm::vec3(currentPos.x, 0.01f, currentPos.z);
}

void EnemyPotato::DestroyShadow() {
    if (m_ShadowNode) {
        GetScene()->QueueDelete(m_ShadowNode);
        m_ShadowNode = nullptr;
    }
}

void EnemyPotato::MoveShadowTo(const glm::vec3& worldPos) {
    if (m_ShadowNode)
        m_ShadowNode->GlobalTransform().Position() =
            glm::vec3(worldPos.x, 0.01f, worldPos.z);
}

void EnemyPotato::StartAttack() {
    m_IsAttacking  = true;
    m_AttackPhase  = PotatoAttackPhase::JUMP_UP;
    m_PhaseTimer   = 0.0f;
    m_JumpStart    = currentPos;

    if (m_Body) m_Body->SetIsSensor(true);

    glm::vec3 dir   = glm::normalize(m_TargetPosition - currentPos);
    float     hDist = glm::length(glm::vec3(m_TargetPosition.x - currentPos.x,
                                            0.0f,
                                            m_TargetPosition.z - currentPos.z));
    m_Apex = currentPos + dir * (hDist * 0.5f) + glm::vec3(0.0f, 5.0f, 0.0f);

    SpawnShadow();
    StopMoving();

    SetLoopingAnimation("attack.001");

    spdlog::info("EnemyPotato: jump attack started, apex ({:.1f},{:.1f},{:.1f})",
                 m_Apex.x, m_Apex.y, m_Apex.z);
}

void EnemyPotato::UpdateAttackSequence() {
    const float dt = Time::Delta();
    m_PhaseTimer  += dt;

    switch (m_AttackPhase) {

    case PotatoAttackPhase::JUMP_UP: {
        constexpr float duration = 0.5f;
        float t      = glm::clamp(m_PhaseTimer / duration, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(m_JumpStart, m_Apex, t);

        myNode->GlobalTransform().Position() = pos;
        MoveShadowTo(pos);

        if (m_PhaseTimer >= duration) {
            m_AttackPhase = PotatoAttackPhase::CHASE;
            m_PhaseTimer  = 0.0f;
        }
        break;
    }

    case PotatoAttackPhase::CHASE: {
        glm::vec3 curPos     = myNode->GlobalTransform().Position();
        glm::vec3 targetAtApex = glm::vec3(m_TargetPosition.x, m_Apex.y, m_TargetPosition.z);

        myNode->GlobalTransform().Position() = glm::mix(curPos, targetAtApex, dt * 2.0f);

        if (m_ShadowNode && m_TargetNode) {
            glm::vec3 playerFloor = m_TargetNode->GlobalTransform().Position();
            glm::vec3 curShadow   = m_ShadowNode->GlobalTransform().Position();
            glm::vec3 newShadow   = glm::mix(curShadow,
                                              glm::vec3(playerFloor.x, 0.01f, playerFloor.z),
                                              dt * 10.0f);
            m_ShadowNode->GlobalTransform().Position() = newShadow;
        }

        if (m_PhaseTimer >= m_ShadowChaseDuration) {
            m_FinalShadowPos = m_ShadowNode
                               ? m_ShadowNode->GlobalTransform().Position()
                               : myNode->GlobalTransform().Position();
            m_AttackPhase = PotatoAttackPhase::STAY;
            m_PhaseTimer  = 0.0f;
        }
        break;
    }

    case PotatoAttackPhase::STAY: {
        glm::vec3 hoverTarget = m_FinalShadowPos + glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 curPos      = myNode->GlobalTransform().Position();

        myNode->GlobalTransform().Position() = glm::mix(curPos, hoverTarget, dt);
        if (m_ShadowNode)
            m_ShadowNode->GlobalTransform().Position() = m_FinalShadowPos;

        if (m_PhaseTimer >= m_ShadowStayDuration) {
            m_PlungeStart = myNode->GlobalTransform().Position();
            m_AttackPhase = PotatoAttackPhase::PLUNGE;
            m_PhaseTimer  = 0.0f;
        }
        break;
    }

    case PotatoAttackPhase::PLUNGE: {
        constexpr float duration = 0.2f;
        float t      = glm::clamp(m_PhaseTimer / duration, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(m_PlungeStart, m_FinalShadowPos, t);

        myNode->GlobalTransform().Position() = pos;

        if (m_PhaseTimer >= duration) {
            if (m_TargetNode) {
                glm::vec3 playerPos = m_TargetNode->GlobalTransform().Position();
                if (glm::distance(playerPos, m_FinalShadowPos) <= 1.5f) {
                    auto* pc = m_TargetNode->GetObject<PlayerController>();
                    if (pc) pc->TakeDamage(m_Damage);
                    spdlog::info("EnemyPotato: plunge hit for {} damage", m_Damage);
                }
            }

            DestroyShadow();

            m_LandStart = myNode->GlobalTransform().Position();
            m_LandTarget = glm::vec3(m_FinalShadowPos.x, m_JumpStart.y, m_FinalShadowPos.z);

            m_AttackPhase = PotatoAttackPhase::LAND;
            m_PhaseTimer  = 0.0f;
        }

        break;
    }
    case PotatoAttackPhase::LAND: {
        constexpr float duration = 0.3f;
        float t = glm::clamp(m_PhaseTimer / duration, 0.0f, 1.0f);
        // ease out — na początku szybko, na końcu zwalnia
        float tEased = 1.0f - (1.0f - t) * (1.0f - t);

        glm::vec3 pos = glm::mix(m_LandStart, m_LandTarget, tEased);
        myNode->GlobalTransform().Position() = pos;

        if (m_Body) {
            m_Body->SetPosition(pos);
            m_Body->SetLinearVelocity(glm::vec3(0.0f));
            m_Body->SetAngularVelocity(glm::vec3(0.0f));
        }

        if (m_PhaseTimer >= duration) {
            if (m_Body) {
                m_Body->SetIsSensor(false);
                m_Body->SetPosition(m_LandTarget);
                m_Body->SetLinearVelocity(glm::vec3(0.0f));
            }
            myNode->GlobalTransform().Position() = m_LandTarget;
            currentPos = m_LandTarget;

            m_IsAttacking    = false;
            m_AttackCooldown = 10.0f;
            m_AttackPhase    = PotatoAttackPhase::NONE;
            spdlog::info("EnemyPotato: landing finished");
        }
        break;
    }
    default: break;
    }
}

void EnemyPotato::Update() {
    EnsureBody();
    LockXZRotation();
    if (!m_TargetNode) return;

    if (!m_AnimInitialized) {
        m_AnimInitialized = true;
        AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
        if (anim) {
            SetAttackAnimation(anim);
            for (auto& a : anim->animations)
                spdlog::info("EnemyPotato: animation: '{}'", a.data.name);
        } else {
            spdlog::error("EnemyPotato: AnimationComponent NOT FOUND");
        }
    }

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    if (!myNode) return;

    if (m_IsAttacking) {
        currentPos = myNode->GlobalTransform().Position();
        UpdateAttackAnimation();
        UpdateAttackSequence();

        if (m_Body) {
            m_Body->SetLinearVelocity(glm::vec3(0.0f));
            m_Body->SetAngularVelocity(glm::vec3(0.0f));
            m_Body->SetPosition(myNode->GlobalTransform().Position());
        }
        return;
    }

    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateStatusEffects();
    UpdateAttackAnimation();
    if (m_InAttackAnimation) { StopMoving(); return; }

    if (IsConfused()) {
        currentState = States::PATROLLING;
    }
    else if (isPlayerInRoom) {
        glm::vec3 diff = currentPos - m_TargetPosition;
        diff.y = 0.0f;
        float dist = glm::length(diff);

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
        DirectChase();
        SetLoopingAnimation("walk.001");
        break;

    case States::ATTACKING:
        if (m_AttackCooldown <= 0.0f) {
            StartAttack();
        } else {
            StopMoving();
            m_AttackCooldown -= Time::Delta();
            SetLoopingAnimation("idle.001");
        }
        break;

    case States::FLEEING:
        Flee();
        SetLoopingAnimation("walk.001");
        break;
    }
}