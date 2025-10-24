#pragma once

#include <GameObject.h>
#include <glm/glm.hpp>

class Camera : public GameObject {
private:
	float fovy;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	static Camera* mainCamera;
public:
	Camera(float fovy, float aspectRatio, float nearPlane, float farPlane);

	glm::mat4 ViewMatrix() const;
	glm::mat4 ProjectionMatrix() const;
	glm::mat4 ViewProjectionMatrix() const;

	static Camera* GetMainCamera();
};