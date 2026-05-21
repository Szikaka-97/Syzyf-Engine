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

///pathfinding and movement


class AiSimplified : public GameObject {
private:
    float m_Speed;               
    float m_RotationSpeed;  
    //float CalculateDistance(glm::vec3 current, glm::vec3 target);
    float m_PatrolTimeout;
	std::vector <glm::vec3> patrolPoints;
	int posIndex;
     float m_PathUpdateTimer;
    float m_ChasePathUpdateTimer = 0.0f;
    glm::vec3 m_LastChasePosition;            
float m_StuckThreshold = 1.5f;       
float m_MinMovementThreshold = 0.2f;  
       


    	void SearchWalkPoint();
	void LookForNextPoint();

         glm::vec3 walkPoint;
    bool walkPointSet;
    float m_AvoidanceRadius = 0.6f;   // promieñ wykrywania
float m_AvoidanceWeight = 0.7f;    // si³a unikania 

     int m_CurrentPathIndex = 0;
     std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& target);
     std::vector<glm::vec3> GetNeighbors(const glm::vec3& node);
     float Heuristic(const glm::vec3& a, const glm::vec3& b);
     bool IsWalkable(const glm::vec3& point);
     glm::vec3 GetNearestWalkable(const glm::vec3& point, float radius = 3.0f);
     glm::vec3 ComputeSteeringDirection(glm::vec3 desiredDir, float speed);
protected: 
    void EnsureBody();  
        void AstarChase();
    void DirectChase();

protected:
    void  SetMovementSpeed(float s) { m_Speed = s; }
    float GetMovementSpeed() const  { return m_Speed; }

public:
    AiSimplified();
    virtual ~AiSimplified();

    //void Update(); 
        void Patrol();
    glm::vec3 m_TargetPosition;    
    glm::vec3 currentPos;
        Physics::Body* m_Body;
    SceneNode* myNode;
        Surface* m_Surface;
        
    NavigationGrid* m_NavGrid = nullptr;
            void Flee();
                void Chase();
    void SetTarget(glm::vec3 target);
    void SetSurface(Surface* surface);
    void UpdateStuckDetection(); 
        void MoveInDirection(const glm::vec3& direction);
                    void StopMoving();
                         std::vector<glm::vec3> m_Path;           //current
                         bool m_UsingAStar = false;   
                            
float m_StuckTimer = 0.0f;   

    void RotateNode(glm::vec3 dir);
        Surface* GetSurface() const {return m_Surface;}
    void SetPatrolPoints(const std::vector<glm::vec2>& points);

    SceneNode* m_TargetNode = nullptr;
void SetTargetNode(SceneNode* node) { m_TargetNode = node; }

   
};