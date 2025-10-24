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
	return glm::lookAt(
		this->GetTransform().GlobalTransform().Position().Value(),
		this->GetTransform().GlobalTransform().Position().Value() + this->GetTransform().GlobalTransform().Forward(),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
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