#pragma once
#include <enemies/EnemyBase.h>
#include <array>
#include <vector>

class EnemyBeetroot : public EnemyBase {
private:
    // ── Tuning ──────────────────────────────────────────────────────────────
    float m_AttackCooldown   = 7.0f;  // seconds between attacks
    int   m_Damage           = 15;    // damage per segment (unused here; dealt by BeetrootSegment)
    float m_AttackDuration   = 2.0f;  // total time to spawn all 8 segments
    float m_FirstSegmentTime = 0.5f;  // delay before the first segment

    // ── State ────────────────────────────────────────────────────────────────
    bool  m_IsAttacking        = false;
    bool  m_HasHealedThisAttack = false;

    // ── Attack sequencing (replaces IEnumerator / StartCoroutine) ────────────
    // Pre-computed absolute timestamps (relative to attack start) for each segment.
    std::array<float, 8> m_SpawnDelays{};
    float                m_AttackElapsed    = 0.0f;
    int                  m_NextSegmentIndex = 0;

    // After all segments are spawned we keep them alive for 1 s then clear.
    bool  m_WaitingClear = false;
    float m_ClearTimer   = 0.0f;

    std::vector<SceneNode*> m_SpawnedSegments;

    // ── Visual resources for segments ────────────────────────────────────────
    Mesh*     m_SegmentMesh     = nullptr;
    Material* m_SegmentMaterial = nullptr;

    // ── Internal helpers ─────────────────────────────────────────────────────
    void ComputeSpawnDelays();
    void StartAttack();
    void UpdateAttackSequence();
    void SpawnSegmentAt(int index, const glm::vec3& dirToPlayer);
    void ClearSegments();

public:
    EnemyBeetroot();

    void Update();

    /// Supply the mesh / material used to render each ground segment.
    void SetSegmentResources(Mesh* mesh, Material* mat);

    /// Called by BeetrootSegment when it damages the player.
    void OnSegmentHitPlayer();
};