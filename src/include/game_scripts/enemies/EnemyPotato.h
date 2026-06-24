#pragma once
#include <./include/game_scripts/enemies/EnemyBase.h>
#include <./include/game_scripts/enemies/loot/LootPool.h>
#include <glm/glm.hpp>

enum class PotatoAttackPhase {
    NONE,
    JUMP_UP,
    CHASE,
    STAY,
    PLUNGE,
};

class EnemyPotato : public EnemyBase {
private:
    float m_AttackCooldown       = 0.0f;
    int   m_Damage               = 30;
    float m_ShadowChaseDuration  = 3.0f;
    float m_ShadowStayDuration   = 2.0f;

    bool             m_IsAttacking  = false;
    PotatoAttackPhase m_AttackPhase = PotatoAttackPhase::NONE;
    float            m_PhaseTimer   = 0.0f;

    glm::vec3 m_JumpStart{};
    glm::vec3 m_Apex{};
    glm::vec3 m_FinalShadowPos{};
    glm::vec3 m_PlungeStart{};

   SceneNode* m_ShadowNode     = nullptr;
    //Mesh*      m_ShadowMesh     = nullptr;
   // Material*  m_ShadowMaterial = nullptr;

    void StartAttack();
    void UpdateAttackSequence();
    void SpawnShadow();
    void DestroyShadow();
    void MoveShadowTo(const glm::vec3& worldPos);

public:
    EnemyPotato();

    void Update();
    LootPool& GetLootPool() override{return LootPool::GetPotatoLootPool();};
   // void SetShadowResources(Mesh* mesh, Material* mat);
};


