#include <AiNode.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <Camera.h>
#include <Graphics.h>
#include <ImGui.h>
#include <random>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "physics/CharacterController.h"
#include "physics/ICollisionReceiver.h"
#include "physics/System.h"
#include "physics/DebugRenderer.h"
#include "physics/Body.h"
#include "physics/Water.h"


AiNode::AiNode()
    : m_Speed(5.0f)
    , m_RotationSpeed(2.0f)
    , m_TargetNode(nullptr) 
    , sightRange(15.0f)          
    , attackRange(5.0f)          
    , walkPointRange(10.0f)      
    , walkPointSet(false)       
{
    myNode = GetNode();
    transform = myNode->GlobalTransform().Position();
	walkPoint = glm::vec3(0.0f);
}

AiNode::~AiNode() {
}

void AiNode::Update() {
    if (!m_TargetNode) {
        Scene* scene = GetScene();
        if (scene) {
            SceneGraphics* graphics = scene->GetGraphics();
            if (graphics) {
                Camera* camera = graphics->GetMainCamera();
                if (camera) {
                    m_TargetNode = camera->GetNode();
                }
            }
        }
        if (!m_TargetNode) return;
    }

    if (!myNode) return;

    transform = myNode->GlobalTransform().Position();

    bool playerInSightRange = glm::distance(glm::vec3(transform), glm::vec3(m_TargetNode->GlobalTransform().Position())) < sightRange;

    bool playerInAttackRange = glm::distance(glm::vec3(transform), glm::vec3(m_TargetNode->GlobalTransform().Position())) < attackRange;
    if (!playerInSightRange && !playerInAttackRange) Patrol();
    if (playerInSightRange && !playerInAttackRange) Chase();
}

void AiNode::SetTarget(SceneNode* target) {
    m_TargetNode = target;
}

void AiNode::Patrol() {
    if (!walkPointSet)
    {
        SearchWalkPoint();
    }
    else {
        glm::vec3 myPos = myNode->GlobalTransform().Position();

        glm::vec3 direction = walkPoint - myPos;
        if (glm::length(direction) > 0.001f) {
            direction = glm::normalize(direction);
            myNode->GlobalTransform().Position() += direction * m_Speed * Time::Delta();
        }

		if (glm::distance(myPos, walkPoint) < 1.0f) {
			walkPointSet = false;
		}
    }
}

void AiNode::Chase() {
    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 myPos = myNode->GlobalTransform().Position();
    glm::vec3 direction = glm::normalize(targetPos - myPos);

    float deltaTime = Time::Delta();
    myNode->GlobalTransform().Position() += direction * m_Speed * deltaTime;

    glm::quat targetRotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));

    glm::quat currentRotation = myNode->GlobalTransform().Rotation();
    glm::quat newRotation = glm::slerp(currentRotation, targetRotation, m_RotationSpeed * deltaTime);
    myNode->GlobalTransform().Rotation() = newRotation;
}

void AiNode::SearchWalkPoint() {
    SceneNode* myNode = GetNode();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-walkPointRange, walkPointRange);

    float randomZ = dist(gen);
    float randomX = dist(gen);
    
	walkPoint = glm::vec3(myNode->GlobalTransform().Position().x + randomX, myNode->GlobalTransform().Position().y, myNode->GlobalTransform().Position().z + randomZ);

	//spdlog::error("Generated walk point: ({}, {}, {})", walkPoint.x, walkPoint.y, walkPoint.z);

	//sprawdŸ czy punkt jest na ziemi
    //auto* physics = GetScene()->GetComponent<Physics::System>();
    //if (physics) {
    //    JPH::RayCastResult result;
    //    JPH::RRayCast ray(
    //        JPH::RVec3(walkPoint.x, walkPoint.y + 10.0f, walkPoint.z),
    //        JPH::Vec3(0, -1, 0)
    //    );
    //    if (physics->GetSystem().GetNarrowPhaseQuery().CastRay(ray, result)) {
    //        JPH::RVec3 hitPoint = ray.GetPointOnRay(result.mFraction);
    //        walkPoint = glm::vec3(hitPoint.GetX(), hitPoint.GetY(), hitPoint.GetZ());
    //        walkPointSet = true;
    //    }
    //    else {
    //        // Jeœli nie trafiliœmy w pod³o¿e, próbujemy ponownie w nastêpnej klatce
    //        walkPointSet = false;
    //    }
    //}
    //else {
    //    // Jeœli nie ma fizyki, zak³adamy, ¿e punkt jest wa¿ny (dla testów)
        walkPointSet = true;
    //}
}