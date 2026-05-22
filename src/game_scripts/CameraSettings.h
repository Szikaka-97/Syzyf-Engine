#pragma once

#include "game_scripts/PlayerController.h"
#include <GameObject.h>
#include <Scene.h>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

class CameraSettings : public GameObject, public ImGuiDrawable {
public:
	glm::vec3 target;
	float height = 5;
	float angleY = 0;
	float angleX = 45;
	float lerpAmount = 0.0f;
public:
	CameraSettings(glm::vec3 target, float height = 5, float angleY = 0, float angleX = 45):
	target(target),
	height(height),
	angleY(angleY),
	angleX(angleX) { }

	CameraSettings(SceneNode* targetNode) : target(0), height(5), angleY(0), angleX(45), lerpAmount(0) {
		
	}

	float RayPlaneIntersection(float height, glm::vec3 start, glm::vec3 direction) {
		glm::vec3 normal{0, 1, 0};

		float denom = glm::dot(normal, direction);

		if (glm::abs(denom) > glm::epsilon<float>()) {
			float t = glm::dot(glm::vec3(0, height, 0) - start, normal) / denom;

			return t;
		}

		return 0;
	}

	void Update() {
		// asm("INT3");

		SceneNode* player = GetScene()->FindNode("Player");

		if (player == nullptr) {
			return;
		}

		this->target = player->GlobalTransform().Position();

		glm::vec3 playerPos = player->GlobalTransform().Position();
		glm::vec3 dir = glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) * (glm::angleAxis(-glm::radians(angleX), glm::vec3(1, 0, 0)) * glm::vec3(0, 0, 1));

		float rayDist = RayPlaneIntersection(this->height, this->target, dir);

		glm::vec3 playerRelativePos = playerPos + dir * rayDist;
		glm::vec3 targetRelativePos = target + dir * rayDist;

		glm::vec3 pos = glm::mix(playerRelativePos, targetRelativePos, lerpAmount);

		GlobalTransform().Position() = pos;

        glm::vec3 lookDir = pos - playerPos;
        if (glm::length(lookDir) > 0.001f) {
            lookDir = glm::normalize(lookDir);
            if (glm::abs(glm::dot(lookDir, glm::vec3(0, 1, 0))) < 0.999f) {
                GlobalTransform().Rotation() = glm::quatLookAt(lookDir, glm::vec3(0, 1, 0));
            }
        }
	}

	void DrawImGui() override {
		ImGui::InputFloat("height", &this->height);
		ImGui::InputFloat("angleY", &this->angleY);
		ImGui::InputFloat("angleX", &this->angleX);
		ImGui::InputFloat("lerpAmount", &this->lerpAmount);

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
