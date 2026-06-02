#pragma once  

#include <GameObject.h>  
#include <Debug.h>  
#include "physics/Body.h"  
#include <vector>  
#include <glm/vec3.hpp>
#include "physics/DebugRenderer.h"  
#include "Mesh.h"  
#include "Surface.h" 

class AiSimplified : public GameObject {  
private:  
   float m_Speed         = 5.0f;  
   float m_RotationSpeed = 2.0f;  
   float m_PatrolTimeout = 0.0f;  
   //
   std::vector<glm::vec3> m_PatrolPoints;  
   int       m_PosIndex    = 0;  
   glm::vec3 m_WalkPoint   = {};  
   bool      m_WalkPointSet = false;  

   glm::vec3 m_AvoidForce    = {};   
   int       m_AvoidFrameCtr = 0;    

   void SearchWalkPoint();  
   void LookForNextPoint();  
   void UpdateWallAvoidance();       

protected:  
   void EnsureBody();  
   void DirectChase();  

   void  SetMovementSpeed(float s) { m_Speed = s; }  
   float GetMovementSpeed() const  { return m_Speed; }  

public:  
   AiSimplified();  
   virtual ~AiSimplified();  

   glm::vec3       m_TargetPosition = {};  
   glm::vec3       currentPos       = {};  
   Physics::Body*  m_Body           = nullptr;  
   SceneNode*      myNode           = nullptr;

   Surface* m_Surface = nullptr;

   SceneNode*      m_TargetNode     = nullptr;  

   void MoveInDirection(const glm::vec3& direction);  
   void StopMoving();  
   void RotateNode(glm::vec3 dir);  
   void LockXZRotation();  
   void Flee();  

   void ChaseWithSteering(const glm::vec3& flockForce);  

   void Patrol();  
   void SetTarget(glm::vec3 target);  
   void SetSurface(Surface* surface);  
   void SetTargetNode(SceneNode* node) {  
       m_TargetNode = node;  
       if (node) currentPos.y = node->GlobalTransform().Position().y;  
   }  
   void SetPatrolPoints(const std::vector<glm::vec2>& points);  
   Surface* GetSurface() const { return m_Surface; }  

   int m_AvoidInterval = 3;  
};
