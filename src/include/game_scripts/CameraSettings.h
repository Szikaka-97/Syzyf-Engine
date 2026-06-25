#pragma once

#include <GameObject.h>
#include <Debug.h>
#include <cmath>

class CameraSettings : public GameObject, public ImGuiDrawable {
private:
	glm::vec3 target;
	serialized float height = 5;
	serialized float angleY = 0;
	serialized float targetAngleY = 0;
	serialized float angleX = 45;
	serialized float cameraRotationSpeed = 180.0f;
	bool frozen = false;
public:
	CameraSettings();
	
	CameraSettings(glm::vec3 target, float height = 7, float angleY = 0, float angleX = 45);

	CameraSettings(SceneNode* targetNode, float height = 7, float angleY = 0, float angleX = 45);

	void Update();

	void DrawImGui() override;

	void SetHeight(float targetHeight);
	void SetAngleY(float targetAngle);
	void SetTargetAngleY(float targetAngle);

	float GetHeight() const;
	float GetAngleY() const;
};
