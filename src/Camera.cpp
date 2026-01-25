#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>

Camera* Camera::mainCamera = nullptr;

Camera::Perspective::Perspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane):
fovyDegrees(fovyDegrees),
aspectRatio(aspectRatio),
nearPlane(nearPlane),
farPlane(farPlane) { }

Camera::Orthographic::Orthographic(float left, float right, float top, float bottom):
left(left),
right(right),
top(top),
bottom(bottom) {}

Camera::Orthographic::Orthographic(glm::vec2 viewportSize):
left(viewportSize.x / 2.0f),
right(-viewportSize.x / 2.0f),
top(viewportSize.y / 2.0f),
bottom(-viewportSize.y / 2.0f) { }

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
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveData.fovyDegrees;
	}

	return 0;
}
float Camera::GetFovRad() const {
	if (this->type == Camera::CameraType::Perspective) {
		return glm::radians(this->perspectiveData.fovyDegrees);
	}

	return 0;
}
float Camera::GetAspectRatio() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveData.aspectRatio;
	}

	return std::abs((this->orthoData.right - this->orthoData.left) / (this->orthoData.top - this->orthoData.bottom));
}
float Camera::GetNearPlane() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveData.nearPlane;
	}

	return 0;
}
float Camera::GetFarPlane() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveData.farPlane;
	}

	return INFINITY;
}

#warning TODO: Make setters consistent with getters
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

CameraData Camera::GetCameraData() const {
	if (this->type == CameraType::Orthographic) {
		return CameraData(this->orthoData, this->ViewMatrix());
	}
	else {
		return CameraData(this->perspectiveData, this->ViewMatrix());
	}
}

CameraData::CameraData(const Camera::Orthographic& orthoParams, const glm::mat4& cameraTransform):
orthoParams(orthoParams),
type(Camera::CameraType::Orthographic),
cameraTransform(cameraTransform) { }

CameraData::CameraData(const Camera::Perspective& perspectiveParams, const glm::mat4& cameraTransform):
perspectiveParams(perspectiveParams),
type(Camera::CameraType::Perspective),
cameraTransform(cameraTransform) { }

glm::mat4 CameraData::ViewMatrix() const {
	return glm::lookAt(
		glm::vec3(this->cameraTransform[3]),
		glm::vec3(this->cameraTransform[3]) + glm::vec3(this->cameraTransform[3]),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
}
glm::mat4 CameraData::ProjectionMatrix() const {
	if (this->type == Camera::CameraType::Perspective) {
		return glm::perspective(
			glm::radians(this->perspectiveParams.fovyDegrees),
			this->perspectiveParams.aspectRatio,
			this->perspectiveParams.nearPlane,
			this->perspectiveParams.farPlane
		);
	}
	else {
		return glm::ortho(
			this->orthoParams.left,
			this->orthoParams.right,
			this->orthoParams.bottom,
			this->orthoParams.top
		);
	}
}
glm::mat4 CameraData::ViewProjectionMatrix() const {
	return ProjectionMatrix() * ViewMatrix();
}

float CameraData::GetFov() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveParams.fovyDegrees;
	}

	return 0;
}
float CameraData::GetFovRad() const {
	if (this->type == Camera::CameraType::Perspective) {
		return glm::radians(this->perspectiveParams.fovyDegrees);
	}

	return 0;
}
float CameraData::GetAspectRatio() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveParams.aspectRatio;
	}

	return std::abs((this->orthoParams.right - this->orthoParams.left) / (this->orthoParams.top - this->orthoParams.bottom));
}
float CameraData::GetNearPlane() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveParams.nearPlane;
	}

	return 0;
}
float CameraData::GetFarPlane() const {
	if (this->type == Camera::CameraType::Perspective) {
		return this->perspectiveParams.farPlane;
	}

	return INFINITY;
}