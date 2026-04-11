#pragma once

#include <GameObject.h>
#include <Debug.h>
#include "physics/Body.h"
#include <Surface.h>
#include <vector>
#include <glm/vec3.hpp>


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


    void Patrol();
    void Chase();
    //void Attack();
	void SearchWalkPoint();
    void RotateNode(glm::vec3 dir);
    void DrawDebugView();
	void LookForNextPoint();
    float CalculateDistance(glm::vec3 current, glm::vec3 target);
    

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

     std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& target);
     std::vector<glm::vec3> GetNeighbors(const glm::vec3& node);
     float Heuristic(const glm::vec3& a, const glm::vec3& b);
     bool IsWalkable(const glm::vec3& point);
     glm::vec3 GetNearestWalkable(const glm::vec3& point, float radius = 3.0f);


public:
    AiNode();
    virtual ~AiNode();

    void Update(); 

    void SetTarget(SceneNode* target);
    void SetSurface(Surface* surface);
    void SetPatrolPoints(const std::vector<glm::vec2>& points);
};