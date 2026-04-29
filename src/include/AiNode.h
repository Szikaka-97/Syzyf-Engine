#pragma once

#include <GameObject.h>
#include <Debug.h>
#include "physics/Body.h"
#include <Surface.h>
#include <vector>
#include <glm/vec3.hpp>
#include "astar/NavigationGrid.h"
#include "physics/DebugRenderer.h"
#include "Mesh.h"

class Material;
class NavigationGrid;
class AnimationComponent;

class AiNode : public GameObject {
private:
    float m_Speed;               
    float m_RotationSpeed;      
    SceneNode* m_TargetNode;    
    SceneNode* myNode;
	glm::vec3 transform;
    Physics::Body* m_Body;
    Surface* m_Surface;
    float m_PatrolTimeout;
	std::vector <glm::vec3> patrolPoints;
	int posIndex;
    float fov;
    float m_PathUpdateTimer;
    float m_ChasePathUpdateTimer = 0.0f;
    float m_AttackCooldown;
    float m_AttackTimer;
    float m_ProjectileSpeed;
    Mesh* m_ProjectileMesh;
    Material* m_ProjectileMaterial; 
    int m_hp;
    NavigationGrid* m_NavGrid = nullptr;
    AnimationComponent* m_AttackAnimation = nullptr;

    void Patrol();
    void Chase();
    void AstarChase();
    void Attack();

    void Flee();
    
    void Die();

    void MoveInDirection(const glm::vec3& direction);
    void StopMoving();


    void SpawnProjectile(const glm::vec3& targetPos);
	void SearchWalkPoint();
    void RotateNode(glm::vec3 dir);
	void LookForNextPoint();
    //float CalculateDistance(glm::vec3 current, glm::vec3 target);
    

     glm::vec3 walkPoint;
    bool walkPointSet;
     float walkPointRange;
     float timeBetweenAttacks;
    bool alreadyAttacked;
    float sightRange = 10.0f;
    float attackRange = 5.0f;
     bool playerInSightRange, playerInAttackRange;
     std::vector<glm::vec3> m_Path;           //current
     int m_CurrentPathIndex = 0;
     bool isPlayerInRoom = false;   
     int m_RoomID;
     std::string m_CurrentAnimation;   
     bool m_InAttackAnimation = false;
float m_AttackAnimationDuration = 1.0f;
float m_AttackAnimationElapsed = 0.0f;


     std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& target);
     std::vector<glm::vec3> GetNeighbors(const glm::vec3& node);
     float Heuristic(const glm::vec3& a, const glm::vec3& b);
     bool IsWalkable(const glm::vec3& point);
     glm::vec3 GetNearestWalkable(const glm::vec3& point, float radius = 3.0f);

     void UpdateAttackAnimation();
     void SetAnimation(const std::string& name);
public:
    AiNode();
    virtual ~AiNode();

    void Update(); 

    void SetTarget(SceneNode* target);
    void SetSurface(Surface* surface);
    void SetRoomID(int id) { m_RoomID = id; }
    int GetID() const { return m_RoomID; }
    Surface* GetSurface() const {return m_Surface;}
    void SetPatrolPoints(const std::vector<glm::vec2>& points);
    void SetProjectileResources(Mesh* mesh, Material* material);
    void SetAttackCooldown(float cooldown);
    void SetAttackAnimation(AnimationComponent* anim);
    void PlayAttackAnimation(std::string name);

    void OnPlayerEnteredRoom();
    void OnPlayerExitedRoom();

    void DrawDebugView();

    void TakeDamage(int damage);
};
