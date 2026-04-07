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

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>

AiNode::AiNode()
    : m_Speed(5.0f)
    , m_RotationSpeed(2.0f)
    , m_TargetNode(nullptr) 
    , sightRange(15.0f)          
    , attackRange(5.0f)          
    , walkPointRange(10.0f)      
    , walkPointSet(false)    
    , m_Body(nullptr)
	, fov(glm::radians(180.0f))
{
    patrolPoints.clear();
    m_Surface = nullptr;
    myNode = GetNode(); 
    if (myNode) {
        m_Body = myNode->GetObject<Physics::Body>();
        transform = m_Body ? m_Body->GetPosition() : myNode->GlobalTransform().Position();
    }
	walkPoint = glm::vec3(0.0f);
    m_PatrolTimeout = 0.0f;

	SetSurface(nullptr);
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

    transform = m_Body->GetPosition();
    myNode->GlobalTransform().Position() = transform;
    myNode->GlobalTransform().Rotation() = m_Body->GetRotation();
    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 dirToTarget = targetPos - transform;
    float dist = glm::length(dirToTarget);
    bool canSeePlayer = false;

    if (dist < sightRange) {
        dirToTarget /= dist;
        glm::mat3 rotMat = glm::toMat3(m_Body->GetRotation());
        glm::vec3 forward = rotMat * glm::vec3(0, 0, 1);
        forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
        glm::vec3 dirFlat = glm::normalize(glm::vec3(dirToTarget.x, 0, dirToTarget.z));
        float dot = glm::dot(forward, dirFlat);
        float angle = acos(glm::clamp(dot, -1.0f, 1.0f));
        if (angle <= fov / 2.0f) {
            canSeePlayer = true;  // bez raycasta ?
            //pdlog::info("can see player");
        }
    }

    bool playerInAttackRange = canSeePlayer && dist < attackRange;

    if (!canSeePlayer && !playerInAttackRange) {
        Patrol();
    }
    else if (canSeePlayer && !playerInAttackRange) {
        Chase();
    }
    else if (canSeePlayer && playerInAttackRange) {
        // Attack();
    }

    DrawDebugView();
}

void AiNode::SetTarget(SceneNode* target) {
    m_TargetNode = target;
}

void AiNode::SetPatrolPoints(const std::vector<glm::vec2>& points) {
	//patrolPoints = points;

	for (const auto& point : points) {
		patrolPoints.push_back(glm::vec3(point.x, m_Surface->GetGroundHeight(point.x,point.y), point.y));
	}
    
}

void AiNode::SetSurface(Surface* surface) {
    if (surface) {
        m_Surface = surface;
    }
    else {
        auto surfaces = GetScene()->FindObjectsOfType<Surface>();
        if (!surfaces.empty()) {
            m_Surface = surfaces[0];
        }
        else {
            spdlog::error("AiNode: No Surface component found in scene");
        }
    }
	
}

void AiNode::Patrol() {
    if (!walkPointSet)
    {
        SearchWalkPoint();
    }
    else {
        //glm::vec3 myPos = transform;

        glm::vec3 dir = walkPoint - transform;
        float distance = glm::length(dir);

        m_PatrolTimeout += Time::Delta();
        if (m_PatrolTimeout > 5.0f) {
			spdlog::warn("AiNode: Patrol timeout reached, resetting walk point");
            walkPointSet = false;
            m_PatrolTimeout = 0.0f;
            return;
        }

        if (distance > 0.5f) {
            dir /= distance;

            //gravity
            glm::vec3 currentVel = m_Body->GetLinearVelocity();
            glm::vec3 newVel = dir * m_Speed;
            newVel.y = currentVel.y; 
            m_Body->SetLinearVelocity(newVel);

			// only yaw rotation
            RotateNode(dir);
        }
        else {
            glm::vec3 currentVel = m_Body->GetLinearVelocity();
            m_Body->SetLinearVelocity(glm::vec3(0, currentVel.y, 0));
            walkPointSet = false;
        }
    }
}

void AiNode::Chase() {
    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 dir = targetPos - transform;
    float distance = glm::length(dir);
    if (distance > 0.1f) {
        dir /= distance;
        glm::vec3 currentVel = m_Body->GetLinearVelocity();
        glm::vec3 newVel = dir * m_Speed;
        newVel.y = currentVel.y;
        m_Body->SetLinearVelocity(newVel);

		// only yaw rotation
		RotateNode(dir);
    }
    else {
        glm::vec3 currentVel = m_Body->GetLinearVelocity();
        m_Body->SetLinearVelocity(glm::vec3(0, currentVel.y, 0));
    }
}

void AiNode::RotateNode(glm::vec3 dir) {
	if (glm::length(dir) > 0.01f) {
		dir = glm::normalize(dir);
		float targetYaw = atan2(dir.x, dir.z);
		glm::quat targetRot = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
		glm::quat currentRot = myNode->GlobalTransform().Rotation();
		glm::quat newRot = glm::slerp(currentRot, targetRot, m_RotationSpeed * Time::Delta());
		m_Body->SetRotation(newRot);
		myNode->GlobalTransform().Rotation() = newRot;
        m_Body->SetAngularVelocity(glm::vec3(0, 0, 0));
	}
}

void AiNode::SearchWalkPoint() {
    if (m_Surface) {
		if (patrolPoints.size() > 0) {
			LookForNextPoint();
		}
        else {
            walkPoint = m_Surface->GetRandomWalkPoint(transform, walkPointRange);
            walkPointSet = true;
        }
        
    }
    else {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-walkPointRange, walkPointRange);

        float randomZ = dist(gen);
        float randomX = dist(gen);

        glm::vec3 candidate(transform.x + randomX, transform.y + 10.0f, transform.z + randomZ);
       
        //ground check
        ///unused
        auto* physics = GetScene()->GetComponent<Physics::System>();
        if (physics) {
            JPH::RRayCast ray(JPH::RVec3(candidate.x, candidate.y, candidate.z), JPH::Vec3(0, -1, 0));
            JPH::RayCastResult result;
            if (physics->GetSystem().GetNarrowPhaseQuery().CastRay(ray, result)) {
                JPH::RVec3 hit = ray.GetPointOnRay(result.mFraction);
                walkPoint = glm::vec3(hit.GetX(), hit.GetY(), hit.GetZ());
                walkPointSet = true;
                spdlog::error("XXXXGenerated walk point: ({}, {}, {})", walkPoint.x, walkPoint.y, walkPoint.z);
            }
            else {
                walkPointSet = false; 
                spdlog::error("XXXXfailed");
            }
            
        }
        else {
            // fallback – no physics
            walkPoint = candidate;
            walkPoint.y = transform.y;
            walkPointSet = true;
        }
        ///
    }
}

void AiNode::LookForNextPoint() {
    posIndex++;
	if (posIndex == patrolPoints.size()) {
		posIndex = 0;
	}
	walkPoint = patrolPoints[posIndex];
	walkPointSet = true;
}
void AiNode::DrawDebugView() {
    if (!myNode) return;

    auto* scene = GetScene();
    auto* debugRenderer = scene ? scene->GetComponent<Physics::DebugRenderer>() : nullptr;
    if (!debugRenderer) {
        return;
    }

    int segments = 24;

    glm::quat rotation = myNode->GlobalTransform().Rotation();
    glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
    forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

    //glm::vec3 pos = transform;

    std::vector<glm::vec3> arcPoints;
    float startAngle = atan2(forward.x, forward.z) - fov / 2.0f;
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        float angle = startAngle + t * fov;
        float x = sightRange * sin(angle);
        float z = sightRange * cos(angle);
        arcPoints.push_back(transform + glm::vec3(x, 0, z));
    }

    for (const auto& p : arcPoints) {
        debugRenderer->DrawLine(JPH::Vec3(transform.x, transform.y, transform.z), JPH::Vec3(p.x, p.y, p.z), JPH::Color::sPurple);
    }

    for (size_t i = 0; i < arcPoints.size() - 1; ++i) {
        debugRenderer->DrawLine(JPH::Vec3(arcPoints[i].x, arcPoints[i].y, arcPoints[i].z),
            JPH::Vec3(arcPoints[i + 1].x, arcPoints[i + 1].y, arcPoints[i + 1].z),
            JPH::Color::sPurple);
    }
}

