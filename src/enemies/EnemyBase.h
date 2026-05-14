#pragma once

#include <AiSimplified.h>

enum States { ATTACKING, FLEEING, CHASING, PATROLLING, AVOIDING_OBSTACLE };

class EnemyBase : public AiSimplified {
private:
    float fov = glm::radians(180.0f);
    float m_AttackCooldown;
    float m_AttackTimer;
    float m_ProjectileSpeed;
    Mesh* m_ProjectileMesh;
    Material* m_ProjectileMaterial;
    AnimationComponent* m_AttackAnimation = nullptr;
    void Die();
    void SpawnProjectile(const glm::vec3& targetPos);
    float timeBetweenAttacks;
    bool alreadyAttacked;
    float sightRange = 10.0f;
    bool playerInSightRange, playerInAttackRange;
    int m_RoomID = 0;
    std::string m_CurrentAnimation;
    float m_AttackAnimationDuration = 1.0f;
    float m_AttackAnimationElapsed = 0.0f;
    void SetAnimation(const std::string& name);
    bool CanSeePlayer() const;

    struct BurnState {
        bool  active        = false;
        float remainingTime = 0.0f;
        float damage        = 0.0f;   // per tick
        float interval      = 1.0f;   // between ticks
        float intervalTimer = 0.0f;
    } m_Burn;
 
    struct PetrifyState {
        bool  active        = false;
        float remainingTime = 0.0f;
        float originalSpeed = 0.0f;  
    } m_Petrify;
 
    struct ConfuseState {
        bool  active        = false;
        float remainingTime = 0.0f;
        bool  isPrecise     = false;
    } m_Confuse;
 
protected:
    void UpdateStatusEffects();

public:
    EnemyBase();
    ~EnemyBase();

    bool m_InAttackAnimation = false;

    bool isPlayerInRoom = false;

    int m_hp;
    float attackRange = 5.0f;
    States currentState = States::PATROLLING;
    // void Update();
    void SetRoomID(int id) { m_RoomID = id; }
    int GetID() const { return m_RoomID; }
    void SetProjectileResources(Mesh* mesh, Material* material);
    void SetAttackCooldown(float cooldown);
    void SetAttackAnimation(AnimationComponent* anim);
    void PlayAttackAnimation(std::string name);

    void OnPlayerEnteredRoom();
    void OnPlayerExitedRoom();

    void DrawDebugView();

    void TakeDamage(int damage);

    void UpdateAttackAnimation();

    void Attack();
    /// todo
    void DropLoot();

    void ApplyBurn(float damagePerTick, float duration, float interval = 1.0f);
 
    void ApplyPetrify(float slowFactor, float duration);
 
    void ApplyConfuse(float duration, bool isPrecise);
 
    bool IsPetrified() const { return m_Petrify.active; }
    bool IsBurning()   const { return m_Burn.active;    }
    bool IsConfused()  const { return m_Confuse.active; }
};