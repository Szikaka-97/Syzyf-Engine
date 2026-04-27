#include <AiSimplified.h>

#include <Scene.h>
#include <TimeSystem.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <Camera.h>
#include <Graphics.h>
#include <imgui.h>
#include <random>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <limits>
#include <functional>
#include <map>
#include <MeshRenderer.h>
#include "astar/NavigationGrid.h"

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

AiSimplified::AiSimplified()
	: m_Speed(5.0f)
	, m_RotationSpeed(2.0f)
{
	m_Body = nullptr;
	m_Surface = nullptr;
	myNode = GetNode();
	m_Body = myNode->GetObject<Physics::Body>();
}

AiSimplified::~AiSimplified() {
}

void AiSimplified::Update() {
	//if (!m_TargetPosition) return;
	if (!myNode) return;
	glm::vec3 currentPos = myNode->GlobalTransform().Position();
	glm::vec3 dirToTarget = m_TargetPosition - currentPos;
	float distance = glm::length(dirToTarget);
	if (distance < 0.1f) {
		StopMoving();
		return;
	}
	dirToTarget /= distance; // normalize
	MoveInDirection(dirToTarget);
}

void AiSimplified::MoveInDirection(const glm::vec3& direction) {
    if (glm::length(direction) < 0.001f) {
        StopMoving();
        return;
    }
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 currentVel = m_Body->GetLinearVelocity();
    glm::vec3 newVel = dir * m_Speed;
    newVel.y = currentVel.y;

	if (m_Surface) {
        glm::vec3 predictedPos = myNode->GlobalTransform().Position() + newVel * Time::Delta();
        if (!m_Surface->ContainsPoint(predictedPos,0.2)) {
            StopMoving(); 
            return;
        }
    }

    m_Body->SetLinearVelocity(newVel);
    RotateNode(dir);
}

void AiSimplified::StopMoving() {
    glm::vec3 currentVel = m_Body->GetLinearVelocity();
    m_Body->SetLinearVelocity(glm::vec3(0, currentVel.y, 0));
}

	void AiSimplified::RotateNode(glm::vec3 dir) {
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

	void AiSimplified::SetTarget(glm::vec3 target) {
		m_TargetPosition = target;
	}

	void AiSimplified::SetSurface(Surface * surface) {
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