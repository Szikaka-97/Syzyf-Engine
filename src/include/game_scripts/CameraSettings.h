#pragma once

#include <GameObject.h>
#include <Debug.h>
#include <cmath>

class CameraSettings : public GameObject, public ImGuiDrawable {
public:
	glm::vec3 target;
	float height = 5;
	float angleY = 0;
	float angleX = 45;
	float cameraRotationSpeed = 40.0f;
public:
	CameraSettings();
	
	CameraSettings(glm::vec3 target, float height = 7, float angleY = 0, float angleX = 45);

	CameraSettings(SceneNode* targetNode);

	void Update();

	void DrawImGui() override;
};
