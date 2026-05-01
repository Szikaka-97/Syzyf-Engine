#pragma once

#include <AiSimplified.h>

enum States {
    ATTACKING,
    FLEEING,
    CHASING,
    PATROLLING,
    AVOIDING_OBSTACLE
};

class EnemyBase : public AiSimplified {
	private:
		float fov;
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
     int m_RoomID;
     std::string m_CurrentAnimation;   
float m_AttackAnimationDuration = 1.0f;
float m_AttackAnimationElapsed = 0.0f;
     void SetAnimation(const std::string& name);
     bool CanSeePlayer() const;

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
    ///todo 
    void DropLoot();

};