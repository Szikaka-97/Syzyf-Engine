#include <game_scripts/CameraSettings.h>

#include <game_scripts/PlayerController.h>
#include <InputSystem.h>
#include <TimeSystem.h>
#include <MathHelpers.h>

#include <imgui.h>

float RayPlaneIntersection(float height, glm::vec3 start, glm::vec3 direction) {
	glm::vec3 normal{0, 1, 0};

	float denom = glm::dot(normal, direction);

	if (glm::abs(denom) > glm::epsilon<float>()) {
		float t = glm::dot(glm::vec3(0, height, 0) - start, normal) / denom;

		return t;
	}

	return 0;
}

CameraSettings::CameraSettings():
CameraSettings(glm::vec3(0, 0, 0), 5, 0, 45) { }

CameraSettings::CameraSettings(glm::vec3 target, float height, float angleY, float angleX):
target(target),
height(height),
angleY(angleY),
targetAngleY(angleY),
angleX(angleX) { }

CameraSettings::CameraSettings(SceneNode* targetNode, float height, float angleY, float angleX):
CameraSettings(targetNode->GlobalTransform().Position(), height, angleY, angleX) {}

void CameraSettings::Update() {
	if (this->frozen) return;

	auto* player = PlayerController::Instance();

	if (this->angleY == this->targetAngleY) {
		if (GetScene()->Input()->KeyDown(Key::Q)) {
			this->targetAngleY += 45;
		}
		if (GetScene()->Input()->KeyDown(Key::E)) {
			this->targetAngleY -= 45;
		}
	}

	this->targetAngleY = glm::mod(this->targetAngleY, 360.f);

	this->angleY = Math::MoveTowardsDegrees(this->angleY, this->targetAngleY, this->cameraRotationSpeed * Time::Delta());

	if (player == nullptr) {
		return;
	}

	this->target = player->GlobalTransform().Position();
	
	glm::vec3 forward = GlobalTransform().Forward();
	forward.y = 0.0f;

	if (glm::length(forward) < glm::epsilon<float>()) {
		forward = GlobalTransform().Up();
		forward.y = 0.0f;
	}
	forward = glm::normalize(forward);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

	glm::vec3 playerPos = player->GlobalTransform().Position();

	playerPos += (forward + right) * 0.5f;
	
	glm::vec3 dir = glm::angleAxis(glm::radians(angleY), glm::vec3(0, 1, 0)) * (glm::angleAxis(-glm::radians(angleX), glm::vec3(1, 0, 0)) * glm::vec3(0, 0, 1));

	float rayDist = RayPlaneIntersection(this->height, this->target, dir);

	glm::vec3 playerRelativePos = playerPos + dir * rayDist;
	glm::vec3 targetRelativePos = target + dir * rayDist;

	glm::vec3 pos = playerRelativePos;

	GlobalTransform().Position() = pos;

	glm::vec3 lookDir = pos - playerPos;
	if (glm::length(lookDir) > 0.001f) {
		lookDir = glm::normalize(lookDir);

		if (glm::abs(glm::dot(lookDir, glm::vec3(0, 1, 0))) < 0.999f) {
			GlobalTransform().Rotation() = glm::quatLookAt(lookDir, glm::vec3(0, 1, 0));
		}
	}
}

void CameraSettings::DrawImGui() {
	ImGui::InputFloat("height", &this->height);
	ImGui::InputFloat("targetAngleY", &this->targetAngleY);
	ImGui::InputFloat("angleY", &this->angleY);
	ImGui::InputFloat("angleX", &this->angleX);
	ImGui::InputFloat("rotationSpeed", &this->cameraRotationSpeed);
}

void CameraSettings::SetHeight(float targetHeight) {
	this->height = targetHeight;
}

void CameraSettings::SetAngleY(float targetAngle) {
	this->targetAngleY = targetAngle;
	this->angleY = targetAngle;
}

void CameraSettings::SetTargetAngleY(float targetAngle) {
	this->targetAngleY = targetAngle;
}

float CameraSettings::GetHeight() const {
	return this->height;
}

float CameraSettings::GetAngleY() const {
	return this->angleY;
}