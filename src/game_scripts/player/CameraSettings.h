#pragma once

#include "Debug.h"
#include "TimeSystem.h"
#include <GameObject.h>
#include <Scene.h>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class CameraSettings : public GameObject, public ImGuiDrawable {
public:
	glm::vec3 target;
	float height = 9;
	float angleY = 45;
	float angleX = 50;
	float lerpAmount = 5.0f;

    float forwardOffset = 0.5f;
    float rightOffset = 0.5f;
public:
	CameraSettings(glm::vec3 target, float height, float angleY, float angleX):
	target(target),
	height(height),
	angleY(angleY),
	angleX(angleX) { }

    CameraSettings(glm::vec3 target) : target(target) {}
	CameraSettings(SceneNode* targetNode) : target(0) {}

	float RayPlaneIntersection(float height, glm::vec3 start, glm::vec3 direction) {
		glm::vec3 normal{0, 1, 0};

		float denom = glm::dot(normal, direction);

		if (glm::abs(denom) > glm::epsilon<float>()) {
            return height / denom;
		}

		return 0;
	}

	void Update() {
		// asm("INT3");

		SceneNode* player = GetScene()->FindNode("Player");

		if (player == nullptr) {
			return;
		}

		glm::vec3 playerPos = player->GlobalTransform().Position();

        glm::vec3 dir = glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) * (glm::angleAxis(-glm::radians(angleX), glm::vec3(1, 0, 0)) * glm::vec3(0, 0, 1));

        glm::vec3 flatForward(-dir.x, 0.0f, -dir.z);
        if (glm::length(flatForward) > 0.001f) {
            flatForward = glm::normalize(flatForward);
        } else {
            flatForward = glm::vec3(0, 0, -1);
        }

        glm::vec3 flatRight = glm::normalize(glm::cross(flatForward, glm::vec3(0, 1, 0)));

        glm::vec3 desiredTarget = playerPos + (flatForward * forwardOffset) + (flatRight * rightOffset);

        if (glm::length(this->target) < 0.001f) {
            this->target = desiredTarget;
        }
		
        if (this->lerpAmount > 0.001f) {
            this->target = glm::mix(this->target, desiredTarget, 1.0f - glm::exp(-this->lerpAmount * Time::Delta()));
        } else {
            this->target = desiredTarget;
        }

		float rayDist = RayPlaneIntersection(this->height, this->target, dir);
        glm::vec3 pos = this->target + dir * rayDist;
		GlobalTransform().Position() = pos;

        glm::vec3 lookDir = this->target - pos;
        if (glm::length(lookDir) > 0.001f) {
            lookDir = glm::normalize(lookDir);
            if (glm::abs(glm::dot(lookDir, glm::vec3(0, 1, 0))) < 0.999f) {
                GlobalTransform().Rotation() = glm::quatLookAt(-lookDir, glm::vec3(0, 1, 0));
            }
        }
	}

	void DrawImGui() override {
		ImGui::InputFloat("height", &this->height);
		ImGui::InputFloat("angleY", &this->angleY);
		ImGui::InputFloat("angleX", &this->angleX);
		ImGui::InputFloat("lerpAmount", &this->lerpAmount);

        ImGui::InputFloat("forwardOffset", &this->forwardOffset);
        ImGui::InputFloat("rightOffset", &this->rightOffset);

		// glm::vec3 playerPos = GetScene()->FindObjectsOfType<PlayerController>()[0]->GlobalTransform().Position();
		// glm::vec3 dir = glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) * (glm::angleAxis(-glm::radians(angleX), glm::vec3(1, 0, 0)) * glm::vec3(0, 0, 1));

		// float rayDist = RayPlaneIntersection(this->height, this->target, dir);

		// glm::vec3 playerRelativePos = playerPos + dir * rayDist;
		// glm::vec3 targetRelativePos = target + dir * rayDist;

		// glm::vec3 pos = glm::mix(playerRelativePos, targetRelativePos, lerpAmount);

		// ImGui::Text("%f, %f, %f", dir.x, dir.y, dir.z);
		// ImGui::Text("%f, %f, %f", glm::normalize(target - pos).x, glm::normalize(target - pos).y, glm::normalize(target - pos).z);
	}
};
