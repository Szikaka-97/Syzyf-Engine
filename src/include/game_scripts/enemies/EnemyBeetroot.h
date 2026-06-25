#pragma once
#include <./include/game_scripts/enemies/EnemyBase.h>
#include <./include/game_scripts/enemies/loot/LootPool.h>
#include <array>
#include <vector>

class EnemyBeetroot : public EnemyBase {
private:
    float m_AttackCooldown   = 0.0f;
    int   m_Damage           = 15;
    float m_AttackDuration   = 2.0f;
    float m_FirstSegmentTime = 0.5f;
    glm::vec3 m_AttackDir = glm::vec3(0, 0, 1);

    bool  m_IsAttacking        = false;
    bool  m_HasHealedThisAttack = false;

    std::array<float, 8> m_SpawnDelays{};
    float                m_AttackElapsed    = 0.0f;
    int                  m_NextSegmentIndex = 0;

    bool  m_WaitingClear = false;
    float m_ClearTimer   = 0.0f;

    std::vector<SceneNode*> m_SpawnedSegments;

   // Mesh*     m_SegmentMesh     = nullptr;
    //Material* m_SegmentMaterial = nullptr;

    void ComputeSpawnDelays();
    void StartAttack();
    void UpdateAttackSequence();
    void SpawnSegmentAt(int index, const glm::vec3& dirToPlayer);
    void ClearSegments();

public:
    EnemyBeetroot();

    void Update();
    LootPool& GetLootPool() {
    return LootPool::GetBeetrootLootPool();
};
    //void SetSegmentResources(Mesh* mesh, Material* mat);

    void OnSegmentHitPlayer();
};

