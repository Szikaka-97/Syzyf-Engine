#pragma once
#include <enemies/EnemyBase.h>
#include <glm/glm.hpp>

/// Phases of the potato's jump-and-plunge attack.
/// Replaces Unity's IEnumerator AttackSequence coroutine.
enum class PotatoAttackPhase {
    NONE,     ///< Not attacking; cooldown counting down
    JUMP_UP,  ///< Lerp from start → apex  (0.5 s)
    CHASE,    ///< Track player at apex height  (shadowChaseDuration)
    STAY,     ///< Hover above final shadow pos  (shadowStayDuration)
    PLUNGE,   ///< Slam down to shadow  (0.2 s)
};

class EnemyPotato : public EnemyBase {
private:
    // ── Tuning ──────────────────────────────────────────────────────────────
    float m_AttackCooldown       = 10.0f;
    int   m_Damage               = 30;
    float m_ShadowChaseDuration  = 3.0f;
    float m_ShadowStayDuration   = 2.0f;

    // ── Attack state machine ─────────────────────────────────────────────────
    bool             m_IsAttacking  = false;
    PotatoAttackPhase m_AttackPhase = PotatoAttackPhase::NONE;
    float            m_PhaseTimer   = 0.0f;

    // Cached world positions used across phases
    glm::vec3 m_JumpStart{};
    glm::vec3 m_Apex{};
    glm::vec3 m_FinalShadowPos{};
    glm::vec3 m_PlungeStart{};

    // ── Shadow indicator node ────────────────────────────────────────────────
    SceneNode* m_ShadowNode     = nullptr;
    Mesh*      m_ShadowMesh     = nullptr;
    Material*  m_ShadowMaterial = nullptr;

    // ── Internal helpers ─────────────────────────────────────────────────────
    void StartAttack();
    void UpdateAttackSequence();
    void SpawnShadow();
    void DestroyShadow();
    void MoveShadowTo(const glm::vec3& worldPos);

public:
    EnemyPotato();

    void Update();

    /// Supply the mesh / material used to render the ground shadow.
    void SetShadowResources(Mesh* mesh, Material* mat);
};