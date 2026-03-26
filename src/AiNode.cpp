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
    , m_TargetNode(nullptr) {
    myNode = GetNode();
    transform = myNode->GlobalTransform().Position();
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

    //SceneNode* myNode = GetNode();
    if (!myNode) return;

    transform = myNode->GlobalTransform().Position();

   

    //playerInSightRange = Physics.CheckSphere(myNode.position, sightRange, m_TargetNode);
    bool playerInSightRange = glm::distance(glm::vec3(transform), glm::vec3(m_TargetNode->GlobalTransform().Position())) < sightRange;

    //playerInAttackRange = Physics.CheckSphere(myNode.position, attackRange, m_TargetNode);
    bool playerInAttackRange = glm::distance(glm::vec3(transform), glm::vec3(m_TargetNode->GlobalTransform().Position())) < attackRange;
    if (!playerInSightRange && !playerInAttackRange) Patrol();
    if (playerInSightRange && !playerInAttackRange) Chase();
    //if (playerInAttackRange && playerInSightRange) Attack();
}

void AiNode::SetTarget(SceneNode* target) {
    m_TargetNode = target;
}

void AiNode::Patrol() {
    if (!walkPointSet)
    {
        SearchWalkPoint();
    }
    if (walkPointSet) {
        transform = myNode->GlobalTransform().Position();
		myNode->GlobalTransform().Position() = glm::mix(transform, walkPoint, m_Speed * Time::Delta());
    }
    glm::vec3 distanceToWalkPoint = myNode->GlobalTransform().Position() - walkPoint;
    if (glm::length(distanceToWalkPoint) < 1.0f)
    {
        walkPointSet = false;
    }
}

void AiNode::Chase() {
	// Implement chase behavior (e.g., follow the target)
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

    auto* physics = this->GetScene()->GetComponent<Physics::System>();
    
	walkPoint = glm::vec3(myNode->GlobalTransform().Position().x + randomX, myNode->GlobalTransform().Position().y, myNode->GlobalTransform().Position().z + randomZ);
	//if (Physics.Raycast(walkPoint, -transform.up, 2f, whatIsGround))
	//	walkPointSet = true;
}
