#pragma once

#include <GameObject.h>
#include <Debug.h>

class AiNode : public GameObject {
private:
    float m_Speed;               
    float m_RotationSpeed;      
    SceneNode* m_TargetNode;    
    SceneNode* myNode;
	glm::vec3 transform;

    void Patrol();
    void Chase();
    //void Attack();
	void SearchWalkPoint();

     glm::vec3 walkPoint;
    bool walkPointSet;
     float walkPointRange;
     float timeBetweenAttacks;
    bool alreadyAttacked;
    float sightRange = 10.0f;
    float attackRange = 5.0f;
     bool playerInSightRange, playerInAttackRange;

public:
    AiNode();
    virtual ~AiNode();

    void Update(); 

    void SetTarget(SceneNode* target);
};