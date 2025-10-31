#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

Camera* Camera::mainCamera = nullptr;

Camera::Camera(Perspective perspectiveData):
type(CameraType::Perspective),
perspectiveData(perspectiveData),
orthoData() {
	SetAsMainCamera();
}

Camera::Camera(Orthographic orthoData):
type(CameraType::Orthographic),
perspectiveData(),
orthoData(orthoData) {
	SetAsMainCamera();
}

Camera::~Camera() {
	if (this == mainCamera) {
		std::vector<Camera*> cameras = this->GetScene()->FindObjectsOfType<Camera>();
		
		if (cameras.size() > 0) {
			for (int i = 0; i < cameras.size(); i++) {
				if (cameras[i] != this) {
					mainCamera = cameras[i];

					return;
				}
			}

			mainCamera = nullptr;
		}
		else {
			mainCamera = nullptr;
		}
	}
}

void Camera::MakePerspective() {
	this->type = CameraType::Perspective;
}

void Camera::MakePerspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane) {
	this->type = CameraType::Perspective;

	this->perspectiveData.fovyDegrees = fovyDegrees;
	this->perspectiveData.aspectRatio = aspectRatio;
	this->perspectiveData.nearPlane = nearPlane;
	this->perspectiveData.farPlane = farPlane;
}

void Camera::MakeOrtho() {
	this->type = CameraType::Orthographic;
}
void Camera::MakeOrtho(float left, float right, float top, float bottom) {
	this->type = CameraType::Orthographic;

	this->orthoData.left = left;
	this->orthoData.right = right;
	this->orthoData.top = top;
	this->orthoData.bottom = bottom;
}

Camera::CameraType Camera::GetType() const {
	return this->type;
}
void Camera::SetType(Camera::CameraType type) {
	this->type = type;
}

float Camera::GetFov() const {
	return this->perspectiveData.fovyDegrees;
}
float Camera::GetFovRad() const {
	return glm::radians(this->perspectiveData.fovyDegrees);
}
float Camera::GetAspectRatio() const {
	return this->perspectiveData.aspectRatio;
}
float Camera::GetNearPlane() const {
	return this->perspectiveData.nearPlane;
}
float Camera::GetFarPlane() const {
	return this->perspectiveData.farPlane;
}

void Camera::SetFov(float newFov) {
	this->perspectiveData.fovyDegrees = newFov;
}
void Camera::SetFovRad(float newFovRad) {
	this->perspectiveData.fovyDegrees = glm::degrees(newFovRad);
}
void Camera::SetAspectRatio(float newAspectRatio) {
	this->perspectiveData.aspectRatio = newAspectRatio;
}
void Camera::SetNearPlane(float newNearPlane) {
	this->perspectiveData.nearPlane = newNearPlane;
}
void Camera::SetFarPlane(float newFarPlane) {
	this->perspectiveData.farPlane = newFarPlane;
}

float Camera::GetLeftOrthoPlane() const {
	return this->orthoData.left;
}
float Camera::GetRightOrthoPlane() const {
	return this->orthoData.right;
}
float Camera::GetTopOrthoPlane() const {
	return this->orthoData.top;
}
float Camera::GetBottomOrthoPlane() const {
	return this->orthoData.bottom;
}

void Camera::SetLeftOrthoPlane(float newLeft) {
	this->orthoData.left = newLeft;
}
void Camera::SetRightOrthoPlane(float newRight) {
	this->orthoData.right = newRight;
}
void Camera::SetTopOrthoPlane(float newTop) {
	this->orthoData.top = newTop;
}
void Camera::SetBottomOrthoPlane(float newBottom) {
	this->orthoData.bottom = newBottom;
}

glm::mat4 Camera::ViewMatrix() const {
	return glm::lookAt(
		this->GlobalTransform().Position().Value(),
		this->GlobalTransform().Position().Value() + this->GlobalTransform().Forward(),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
}
glm::mat4 Camera::ProjectionMatrix() const {
	if (this->type == CameraType::Perspective) {
		return glm::perspective(
			glm::radians(this->perspectiveData.fovyDegrees),
			this->perspectiveData.aspectRatio,
			this->perspectiveData.nearPlane,
			this->perspectiveData.farPlane
		);
	}
	else {
		return glm::ortho(
			this->orthoData.left,
			this->orthoData.right,
			this->orthoData.bottom,
			this->orthoData.top
		);
	}
}
glm::mat4 Camera::ViewProjectionMatrix() const {
	return ProjectionMatrix() * ViewMatrix();
}

Camera* Camera::GetMainCamera() {
	return mainCamera;
}

void Camera::SetAsMainCamera() {
	mainCamera = this;
}