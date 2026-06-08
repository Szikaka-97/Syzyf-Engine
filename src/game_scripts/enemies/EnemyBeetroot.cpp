#include "./include/game_scripts/enemies/EnemyBeetroot.h"
#include "./include/game_scripts/enemies/BeetrootSegment.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

// ──────────────────────────────────────────────────────────────────────────────
EnemyBeetroot::EnemyBeetroot() {
    attackRange = 6.0f; // matches Unity Awake() override
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::SetSegmentResources(Mesh* mesh, Material* mat) {
    m_SegmentMesh     = mesh;
    m_SegmentMaterial = mat;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Reproduces Unity's spawnDelays[] computation exactly.
//  Mathf.Lerp(a, b, t) == glm::mix(a, b, t) for floats.
void EnemyBeetroot::ComputeSpawnDelays() {
    const float firstDelay     = m_FirstSegmentTime;
    const float remainingTime  = m_AttackDuration - firstDelay;
    const float baseInterval   = remainingTime / 7.0f;

    m_SpawnDelays[0] = firstDelay;
    for (int i = 1; i < 8; ++i) {
        float ratio    = static_cast<float>(i) / 7.0f;
        // Lerp from (fast) 1.5x to (slow) 0.5x so segments accelerate
        float interval = glm::mix(baseInterval * 1.5f, baseInterval * 0.5f, ratio);
        m_SpawnDelays[i] = m_SpawnDelays[i - 1] + interval;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::StartAttack() {
    m_IsAttacking         = true;
    m_HasHealedThisAttack = false;
    m_AttackElapsed       = 0.0f;
    m_NextSegmentIndex    = 0;
    m_WaitingClear        = false;
    m_ClearTimer          = 0.0f;
    ComputeSpawnDelays();
    StopMoving();
    spdlog::info("BeetrootEnemy: attack sequence started");
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::SpawnSegmentAt(int index, const glm::vec3& dirToPlayer) {
    // Segments appear 1 m away from the beetroot, each 1 m further
    glm::vec3 segPos = currentPos + dirToPlayer * (1.0f + static_cast<float>(index));

    auto* segNode = GetScene()->CreateNode("BeetrootSeg_" + std::to_string(index));
    segNode->GlobalTransform().Position() = segPos;

    if (m_SegmentMesh && m_SegmentMaterial)
        segNode->AddObject<MeshRenderer>(m_SegmentMesh, m_SegmentMaterial);

    // Add the trigger-like component that deals damage on proximity
    auto* seg = segNode->AddObject<BeetrootSegment>();
    seg->Initialize(this, m_TargetNode);

    m_SpawnedSegments.push_back(segNode);
    spdlog::debug("BeetrootEnemy: spawned segment {} at ({:.1f},{:.1f},{:.1f})",
                  index, segPos.x, segPos.y, segPos.z);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Replaces the IEnumerator AttackSequence coroutine.
//  Must be called every frame while m_IsAttacking == true.
void EnemyBeetroot::UpdateAttackSequence() {
    m_AttackElapsed += Time::Delta();

    if (!m_WaitingClear) {
        // Direction is captured at attack-start moment; use currentPos snapshot
        glm::vec3 dir(0.0f);
        glm::vec3 toTarget = m_TargetPosition - currentPos;
        if (glm::length(toTarget) > 0.01f)
            dir = glm::normalize(toTarget);

        // Spawn every segment whose timestamp has been reached
        while (m_NextSegmentIndex < 8 &&
               m_AttackElapsed >= m_SpawnDelays[m_NextSegmentIndex]) {
            SpawnSegmentAt(m_NextSegmentIndex, dir);
            ++m_NextSegmentIndex;
        }

        // All 8 spawned → enter the 1-second "stay alive" wait
        if (m_NextSegmentIndex >= 8) {
            m_WaitingClear = true;
            m_ClearTimer   = 0.0f;
        }
    } else {
        // yield return new WaitForSeconds(1f) equivalent
        m_ClearTimer += Time::Delta();
        if (m_ClearTimer >= 1.0f) {
            ClearSegments();
            m_IsAttacking  = false;
            m_AttackCooldown = 7.0f;
            spdlog::info("BeetrootEnemy: attack sequence finished");
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::ClearSegments() {
    for (auto* seg : m_SpawnedSegments) {
        if (seg) GetScene()->QueueDelete(seg);
    }
    m_SpawnedSegments.clear();
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::OnSegmentHitPlayer() {
    if (!m_HasHealedThisAttack) {
        if (m_hp < 100) m_hp += 15;
        m_HasHealedThisAttack = true;
        spdlog::info("BeetrootEnemy: healed 15 HP (segment hit player)");
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void EnemyBeetroot::Update() {
    EnsureBody();
    LockXZRotation();
    EnsureBody();
    if (!m_TargetNode) return;
 
    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    if (!myNode) return;
 
    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
 
    UpdateAttackAnimation();
    if (m_InAttackAnimation) { StopMoving(); return; }

    // ── Normal FSM ──────────────────────────────────────────────────────────
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