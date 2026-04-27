#pragma once

#include <GameObject.h>
#include <Debug.h>
#include "physics/Body.h"
#include <Surface.h>
#include <vector>
#include <glm/vec3.hpp>


class AiSimplified : public GameObject {
private:
    float m_Speed;               
    float m_RotationSpeed;      
    glm::vec3 m_TargetPosition;    
    SceneNode* myNode;
    Physics::Body* m_Body;
    Surface* m_Surface;
    void MoveInDirection(const glm::vec3& direction);
    void StopMoving();

    void RotateNode(glm::vec3 dir);
    
    //float CalculateDistance(glm::vec3 current, glm::vec3 target);
    

public:
    AiSimplified();
    virtual ~AiSimplified();

    void Update(); 

    void SetTarget(glm::vec3 target);
    void SetSurface(Surface* surface);
   
};