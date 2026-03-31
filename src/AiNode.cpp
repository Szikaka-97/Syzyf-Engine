#include <AiNode.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <Camera.h>
#include <Graphics.h>
#include <imgui.h>
#include <random>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
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
    , m_Body(nullptr)
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

    float dist = glm::distance(transform, glm::vec3(m_TargetNode->GlobalTransform().Position()));
    bool playerInSightRange = dist < sightRange;
    bool playerInAttackRange = dist < attackRange;

	DrawDebugView();

    if (!playerInSightRange && !playerInAttackRange) Patrol();
    if (playerInSightRange && !playerInAttackRange) Chase();
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
        /*auto* floorNode = GetScene()->FindNode("/Floor");
        if (floorNode) {
            m_Surface = floorNode->GetObject<Surface>();
            if (m_Surface) {
                spdlog::info("AiNode: Found Surface on Floor node");
            }
            else {
                spdlog::error("AiNode: Floor node has no Surface component");
            }
        }
        else {
            spdlog::error("AiNode: Floor node not found");
        }*/
        auto surfaces = GetScene()->FindObjectsOfType<Surface>();
        if (!surfaces.empty()) {
            m_Surface = surfaces[0];
            spdlog::info("AiNode: Found Surface component in scene");
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
            // fallback � no physics
            walkPoint = candidate;
            walkPoint.y = transform.y;
            walkPointSet = true;
        }
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

    float fov = glm::radians(90.0f);
    float radius = sightRange;
    int segments = 24;

    glm::quat rotation = myNode->GlobalTransform().Rotation();
    glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
    forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));

    glm::vec3 pos = transform;

    std::vector<glm::vec3> arcPoints;
    float startAngle = atan2(forward.x, forward.z) - fov / 2.0f;
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        float angle = startAngle + t * fov;
        float x = radius * sin(angle);
        float z = radius * cos(angle);
        arcPoints.push_back(pos + glm::vec3(x, 0, z));
    }

    for (const auto& p : arcPoints) {
        debugRenderer->DrawLine(JPH::Vec3(pos.x, pos.y, pos.z), JPH::Vec3(p.x, p.y, p.z), JPH::Color::sGreen);
    }

    for (size_t i = 0; i < arcPoints.size() - 1; ++i) {
        debugRenderer->DrawLine(JPH::Vec3(arcPoints[i].x, arcPoints[i].y, arcPoints[i].z),
            JPH::Vec3(arcPoints[i + 1].x, arcPoints[i + 1].y, arcPoints[i + 1].z),
            JPH::Color::sGreen);
    }
}

