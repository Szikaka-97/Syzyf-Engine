#include "./include/game_scripts/enemies/EnemyPotato.h"
#include "game_scripts/PlayerController.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

// ──────────────────────────────────────────────────────────────────────────────
EnemyPotato::EnemyPotato() {
    attackRange = 3.0f; // matches Unity Awake() override
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyPotato::SetShadowResources(Mesh* mesh, Material* mat) {
    m_ShadowMesh     = mesh;
    m_ShadowMaterial = mat;
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyPotato::SpawnShadow() {
    if (!m_ShadowMesh || !m_ShadowMaterial) return;

    m_ShadowNode = GetScene()->CreateNode("PotatoShadow");
    m_ShadowNode->AddObject<MeshRenderer>(m_ShadowMesh, m_ShadowMaterial);
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

// ──────────────────────────────────────────────────────────────────────────────
void EnemyPotato::StartAttack() {
    m_IsAttacking  = true;
    m_AttackPhase  = PotatoAttackPhase::JUMP_UP;
    m_PhaseTimer   = 0.0f;
    m_JumpStart    = currentPos;

    // Compute apex: midpoint horizontally, +5 m vertically
    glm::vec3 dir   = glm::normalize(m_TargetPosition - currentPos);
    float     hDist = glm::length(glm::vec3(m_TargetPosition.x - currentPos.x,
                                            0.0f,
                                            m_TargetPosition.z - currentPos.z));
    m_Apex = currentPos + dir * (hDist * 0.5f) + glm::vec3(0.0f, 5.0f, 0.0f);

    SpawnShadow();
    StopMoving();
    spdlog::info("EnemyPotato: jump attack started, apex ({:.1f},{:.1f},{:.1f})",
                 m_Apex.x, m_Apex.y, m_Apex.z);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Replaces the IEnumerator AttackSequence coroutine.
//  Note: position is set directly on the node (bypassing physics) during
//  the attack.  After PLUNGE, call m_Body->SetPosition(m_FinalShadowPos) if
//  your engine's Physics::Body exposes it, so the physics proxy re-syncs.
void EnemyPotato::UpdateAttackSequence() {
    const float dt = Time::Delta();
    m_PhaseTimer  += dt;

    switch (m_AttackPhase) {

    // ── Phase 1: jump from start to apex in 0.5 s ────────────────────────
    case PotatoAttackPhase::JUMP_UP: {
        constexpr float duration = 0.5f;
        float t      = glm::clamp(m_PhaseTimer / duration, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(m_JumpStart, m_Apex, t);

        myNode->GlobalTransform().Position() = pos;
        MoveShadowTo(pos); // shadow tracks horizontally

        if (m_PhaseTimer >= duration) {
            m_AttackPhase = PotatoAttackPhase::CHASE;
            m_PhaseTimer  = 0.0f;
        }
        break;
    }

    // ── Phase 2: glide toward player (at apex height) for 3 s ────────────
    case PotatoAttackPhase::CHASE: {
        glm::vec3 curPos     = myNode->GlobalTransform().Position();
        glm::vec3 targetAtApex = glm::vec3(m_TargetPosition.x, m_Apex.y, m_TargetPosition.z);

        // Vector3.Lerp per-frame: position = Lerp(position, target, dt * 2)
        myNode->GlobalTransform().Position() = glm::mix(curPos, targetAtApex, dt * 2.0f);

        // Shadow races toward the player on the ground
        if (m_ShadowNode && m_TargetNode) {
            glm::vec3 playerFloor = m_TargetNode->GlobalTransform().Position();
            glm::vec3 curShadow   = m_ShadowNode->GlobalTransform().Position();
            glm::vec3 newShadow   = glm::mix(curShadow,
                                              glm::vec3(playerFloor.x, 0.01f, playerFloor.z),
                                              dt * 10.0f);
            m_ShadowNode->GlobalTransform().Position() = newShadow;
        }

        if (m_PhaseTimer >= m_ShadowChaseDuration) {
            // Snapshot final shadow position for the plunge target
            m_FinalShadowPos = m_ShadowNode
                               ? m_ShadowNode->GlobalTransform().Position()
                               : myNode->GlobalTransform().Position();
            m_AttackPhase = PotatoAttackPhase::STAY;
            m_PhaseTimer  = 0.0f;
        }
        break;
    }

    // ── Phase 3: hover just above the shadow for 2 s ─────────────────────
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

    // ── Phase 4: slam down to the shadow in 0.2 s, then resolve ──────────
    case PotatoAttackPhase::PLUNGE: {
        constexpr float duration = 0.2f;
        float t      = glm::clamp(m_PhaseTimer / duration, 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(m_PlungeStart, m_FinalShadowPos, t);

        myNode->GlobalTransform().Position() = pos;

        if (m_PhaseTimer >= duration) {
            // ── Damage check ──────────────────────────────────────────────
            if (m_TargetNode) {
                glm::vec3 playerPos = m_TargetNode->GlobalTransform().Position();
                if (glm::distance(playerPos, m_FinalShadowPos) <= 1.5f) {
                    auto* pc = m_TargetNode->GetObject<PlayerController>();
                    if (pc) pc->TakeDamage(m_Damage);
                    spdlog::info("EnemyPotato: plunge hit for {} damage", m_Damage);
                }
            }

            DestroyShadow();

            // Re-sync physics body with the visual node position.
            // If Physics::Body exposes SetPosition, call it here:
            //   m_Body->SetPosition(m_FinalShadowPos);
            // Otherwise zero velocity so the body settles naturally:
            if (m_Body) m_Body->SetLinearVelocity(glm::vec3(0.0f));

            m_IsAttacking    = false;
            m_AttackCooldown = 10.0f;
            m_AttackPhase    = PotatoAttackPhase::NONE;
            spdlog::info("EnemyPotato: attack sequence finished");
        }
        break;
    }

    default: break;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyPotato::Update() {
    EnsureBody();
    LockXZRotation();
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
        case States::CHASING:
            DirectChase();
            break;
        case States::ATTACKING:
            if (!m_IsAttacking && m_AttackCooldown <= 0.0f)
                StartAttack();
            else {
                StopMoving();
                m_AttackCooldown -= Time::Delta();
            }
            break;
        case States::FLEEING:
            Flee();
            break;
        
    }
}