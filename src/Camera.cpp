#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

Camera* Camera::mainCamera = nullptr;

Camera::Camera(float fovy, float aspectRatio, float nearPlane, float farPlane):
fovy(fovy),
aspectRatio(aspectRatio),
nearPlane(nearPlane),
farPlane(farPlane) {
	mainCamera = this;
}

glm::mat4 Camera::ViewMatrix() const {
	return this->GetTransform().GlobalTransform();
}
glm::mat4 Camera::ProjectionMatrix() const {
	return glm::perspective(this->fovy, this->aspectRatio, this->nearPlane, this->farPlane);
}
glm::mat4 Camera::ViewProjectionMatrix() const {
	return ProjectionMatrix() * ViewMatrix();
}

Camera* Camera::GetMainCamera() {
	return mainCamera;
}