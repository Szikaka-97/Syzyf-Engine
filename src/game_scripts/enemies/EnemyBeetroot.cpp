#include "./include/game_scripts/enemies/EnemyBeetroot.h"
#include "./include/game_scripts/enemies/BeetrootSegment.h"
#include <MeshRenderer.h>
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

EnemyBeetroot::EnemyBeetroot() {
    attackRange = 8.0f;
    m_Speed = 2.5f;
    SceneNode* enemyModel =
         ResourceDatabase::Global->Get<GltfScene>("./res/models/enemies/burak_macki3_bisect.glb")
             ->Instantiate(GetScene(), GetNode(), "EnemyBeetrootModel");
    enemyModel->LocalTransform().Position() = glm::zero<glm::vec3>();
    AnimationComponent* enemyAnim = GetNode()->GetObjectInChildren<AnimationComponent>();
   // SetAttackAnimation(enemyAnim);
}

void EnemyBeetroot::ComputeSpawnDelays() {
    const float firstDelay     = m_FirstSegmentTime;
    const float remainingTime  = m_AttackDuration - firstDelay;
    const float baseInterval   = remainingTime / 7.0f;

    m_SpawnDelays[0] = firstDelay;
    for (int i = 1; i < 8; ++i) {
        float ratio    = static_cast<float>(i) / 7.0f;
        float interval = glm::mix(baseInterval * 1.5f, baseInterval * 0.5f, ratio);
        m_SpawnDelays[i] = m_SpawnDelays[i - 1] + interval;
    }
}

void EnemyBeetroot::StartAttack() {
    m_IsAttacking         = true;
    m_HasHealedThisAttack = false;
    m_AttackElapsed       = 0.0f;
    m_NextSegmentIndex    = 0;
    m_WaitingClear        = false;
    m_ClearTimer          = 0.0f;
    ComputeSpawnDelays();
    StopMoving();

    if (m_Body) {
        glm::vec3 vel = m_Body->GetLinearVelocity();
        m_Body->SetLinearVelocity(glm::vec3(0.0f, vel.y, 0.0f));
    }

    glm::vec3 toPlayer = m_TargetPosition - currentPos;
    toPlayer.y = 0.0f;
    float len = glm::length(toPlayer);
    m_AttackDir = (len > 0.01f) ? (toPlayer / len) : glm::vec3(0, 0, 1);
SetLoopingAnimation("attack.001");

    spdlog::info("BeetrootEnemy: attack sequence started, dir=({:.2f},{:.2f},{:.2f})",
                 m_AttackDir.x, m_AttackDir.y, m_AttackDir.z);
}

void EnemyBeetroot::SpawnSegmentAt(int index, const glm::vec3& dirToPlayer) {
    glm::vec3 segPos = currentPos + dirToPlayer * (1.0f + static_cast<float>(index));

    auto* segNode = GetScene()->CreateNode("BeetrootSeg_" + std::to_string(index));
    segNode->GlobalTransform().Position() = segPos;

    auto* seg = segNode->AddObject<BeetrootSegment>();
    seg->Initialize(this, m_TargetNode);

    m_SpawnedSegments.push_back(segNode);
    spdlog::debug("BeetrootEnemy: spawned segment {} at ({:.1f},{:.1f},{:.1f})",
                  index, segPos.x, segPos.y, segPos.z);
}

void EnemyBeetroot::UpdateAttackSequence() {
    m_AttackElapsed += Time::Delta();

    if (!m_WaitingClear) {
        while (m_NextSegmentIndex < 8 &&
               m_AttackElapsed >= m_SpawnDelays[m_NextSegmentIndex]) {
            SpawnSegmentAt(m_NextSegmentIndex, m_AttackDir);
            ++m_NextSegmentIndex;
        }

        if (m_NextSegmentIndex >= 8) {
            m_WaitingClear = true;
            m_ClearTimer   = 0.0f;
        }
    } else {
        m_ClearTimer += Time::Delta();
        if (m_ClearTimer >= 1.0f) {
            ClearSegments();
            m_IsAttacking    = false;
            m_AttackCooldown = 7.0f;
            spdlog::info("BeetrootEnemy: attack sequence finished");
        }
    }
}

void EnemyBeetroot::ClearSegments() {
    for (auto* seg : m_SpawnedSegments) {
        if (seg) GetScene()->QueueDelete(seg);
    }
    m_SpawnedSegments.clear();
}

void EnemyBeetroot::OnSegmentHitPlayer() {
    if (!m_HasHealedThisAttack) {
        if (m_hp < 100) m_hp += 15;
        m_HasHealedThisAttack = true;
        spdlog::info("BeetrootEnemy: healed 15 HP (segment hit player)");
    }
}

void EnemyBeetroot::Update() {
    EnsureBody();
    LockXZRotation();
    if (!m_Body) { spdlog::error("EnemyBeetroot: no body"); return; }
    if (!m_TargetNode) return;

    if (!m_AnimInitialized) {
        m_AnimInitialized = true;
        AnimationComponent* anim = GetNode()->GetObjectInChildren<AnimationComponent>();
        if (anim) {
            SetAttackAnimation(anim);
            spdlog::info("EnemyBeetroot: AnimationComponent found and set");
        } else {
            spdlog::error("EnemyBeetroot: AnimationComponent NOT FOUND in children");
        }
    }

    m_TargetPosition = m_TargetNode->GlobalTransform().Position();
    if (!myNode) return;

    currentPos = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = currentPos;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();

    UpdateStatusEffects();
    UpdateAttackAnimation();
    if (m_InAttackAnimation) { StopMoving(); return; }

    if (!m_IsAttacking && m_AttackCooldown > 0.0f) {
        m_AttackCooldown -= Time::Delta();
    }

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

    case States::CHASING: {
        float dist = glm::distance(currentPos, m_TargetPosition);
        if (dist > 2.5f) {
            DirectChase();
            SetLoopingAnimation("walk.001");
        } else {
            StopMoving();
            glm::vec3 toPlayer = m_TargetPosition - currentPos;
            toPlayer.y = 0.0f;
            if (glm::length(toPlayer) > 0.01f)
                RotateNode(glm::normalize(toPlayer));
            SetLoopingAnimation("idle.001");
        }
        break;
    }

    case States::ATTACKING:
        StopMoving();
        if (m_IsAttacking) {
            SetLoopingAnimation("attack.001");
            UpdateAttackSequence();
        } else if (m_AttackCooldown <= 0.0f) {
            StartAttack();
        } else {
            SetLoopingAnimation("idle.001");
        }
        break;

    default:
        break;
    }
}