#include <Camera.h>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <Graphics.h>
#include <Viewport.h>
#include <Layer.h>

Camera::Orthographic MakeOrthoFromPerspective(const Camera::Perspective& perp) {
	static constexpr float typicalZ = 5;

	return {
		glm::tan(glm::radians(perp.fovyDegrees)) * perp.aspectRatio * typicalZ,
		-glm::tan(glm::radians(perp.fovyDegrees)) * perp.aspectRatio * typicalZ,
		glm::tan(glm::radians(perp.fovyDegrees)) * typicalZ,
		-glm::tan(glm::radians(perp.fovyDegrees)) * typicalZ,
		perp.nearPlane,
		perp.farPlane
	};
}

Camera::Perspective MakePerspectiveFromOrtho(const Camera::Orthographic& ortho) {
	static constexpr float typicalZ = 5;
	
	float size = ortho.top - ortho.bottom;
	float orthoAspec = (ortho.left - ortho.right) / size;


	return Camera::Perspective(glm::degrees(glm::atan(ortho.left, typicalZ)), orthoAspec, ortho.znear, ortho.zfar);
}

RenderPassType Camera::DefaultCameraPasses = RenderPassType::DepthPrepass | RenderPassType::Color;
RenderPassType Camera::DefaultMainCameraPasses = (
	RenderPassType::Color
	|
	RenderPassType::DepthPrepass
	|
	RenderPassType::Transparent
	|
	RenderPassType::Volumetric
	|
	RenderPassType::Mask
);

Camera::Perspective::Perspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane):
fovyDegrees(fovyDegrees),
aspectRatio(aspectRatio),
nearPlane(nearPlane),
farPlane(farPlane) { }

Camera::Orthographic::Orthographic(float left, float right, float top, float bottom, float znear, float zfar):
left(left),
right(right),
top(top),
bottom(bottom),
znear(znear),
zfar(zfar) {}

Camera::Orthographic::Orthographic(glm::vec2 viewportSize):
left(viewportSize.x / 2.0f),
right(-viewportSize.x / 2.0f),
top(viewportSize.y / 2.0f),
bottom(-viewportSize.y / 2.0f) { }

Camera::Camera():
type(CameraType::Perspective),
perspectiveData(Camera::Perspective()),
orthoData(),
layerMask(LayerMask::All),
priority(0) { }

Camera::Camera(Perspective perspectiveData):
type(CameraType::Perspective),
perspectiveData(perspectiveData),
orthoData(MakeOrthoFromPerspective(perspectiveData)),
layerMask(LayerMask::All),
priority(0),
passes(RenderPassType::DepthPrepass | RenderPassType::Color) {
	if (GetScene()->GetGraphics() && GetScene()->GetGraphics()->GetMainCamera() == nullptr) {
		SetAsMainCamera();
	}
}

Camera::Camera(Orthographic orthoData):
type(CameraType::Orthographic),
perspectiveData(MakePerspectiveFromOrtho(orthoData)),
orthoData(orthoData),
layerMask(LayerMask::All),
priority(0),
passes(RenderPassType::DepthPrepass | RenderPassType::Color) {
	if (GetScene()->GetGraphics() && GetScene()->GetGraphics()->GetMainCamera() == nullptr) {
		SetAsMainCamera();
	}
}

Camera::~Camera() {
	if (this == this->GetScene()->GetGraphics()->GetMainCamera()) {
		std::vector<Camera*> cameras = this->GetScene()->FindObjectsOfType<Camera>();
		
		if (cameras.size() > 0) {
			for (int i = 0; i < cameras.size(); i++) {
				if (cameras[i] != this) {
					this->GetScene()->GetGraphics()->SetMainCamera(cameras[i]);

					return;
				}
			}
		}

		this->GetScene()->GetGraphics()->SetMainCamera(nullptr);
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
void Camera::MakeOrtho(float left, float right, float top, float bottom, float znear, float zfar) {
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
	if (this->renderTarget) {
		return (float) this->renderTarget->GetSize().x / this->renderTarget->GetSize().y;
	}

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

void Camera::SetFov(float newFov) {
	this->perspectiveData.fovyDegrees = newFov;
}
void Camera::SetFovRad(float newFovRad) {
	this->perspectiveData.fovyDegrees = glm::degrees(newFovRad);
}
void Camera::SetAspectRatio(float newAspectRatio) {
	this->perspectiveData.aspectRatio = newAspectRatio;

	const float orthoHeight = this->orthoData.top - this->orthoData.bottom;
	const float correctWidth = orthoHeight * newAspectRatio;

	this->orthoData.right = correctWidth / 2;
	this->orthoData.left = correctWidth / -2;
}
void Camera::SetNearPlane(float newNearPlane) {
	this->perspectiveData.nearPlane = newNearPlane;
	this->orthoData.znear = newNearPlane;
}
void Camera::SetFarPlane(float newFarPlane) {
	this->perspectiveData.farPlane = newFarPlane;
	this->orthoData.zfar = newFarPlane;
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
			GetAspectRatio(),
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

Viewport* Camera::GetRenderTarget() const {
	return this->renderTarget;
}

void Camera::SetRenderTarget(Viewport* viewport) {
	this->renderTarget = viewport;
}

uint32_t Camera::GetLayerMask() const {
	return this->layerMask;
}
bool Camera::TestLayer(uint8_t layer) {
	return this->layerMask.Test(layer);
}

int Camera::GetPriority() const {
	return this->priority;
}
void Camera::SetPriority(int priority) {
	this->priority = priority;
}

void Camera::SetLayerMask(LayerMask newMask) {
	this->layerMask = newMask;
}

void Camera::AddLayerToMask(uint8_t layer) {
	this->layerMask |= (1 << layer);
}

void Camera::RemoveLayerFromMask(uint8_t layer) {
	this-> layerMask &= ~(1 << layer);
}

RenderPassType Camera::GetPasses() const {
	return this->passes;
}

void Camera::SetPasses(RenderPassType passes) {
	this->passes = passes;
}

bool Camera::HasPass(RenderPassType pass) {
	return (this->passes & pass) == pass;
}

void Camera::AddPass(RenderPassType pass) {
	this->passes |= pass;
}

void Camera::RemovePass(RenderPassType pass) {
	this->passes &= ~pass;
}

void Camera::SetAsMainCamera() {
	if (GetScene()->GetGraphics()) {
		if (GetScene()->GetGraphics()->GetMainCamera()) {
			GetScene()->GetGraphics()->GetMainCamera()->SetPasses(DefaultCameraPasses);
		}

		GetScene()->GetGraphics()->SetMainCamera(this);

		this->passes = DefaultMainCameraPasses;
	}
}

CameraData Camera::GetCameraData() const {
	if (this->type == CameraType::Orthographic) {
		return CameraData(this->orthoData, this->ViewMatrix());
	}
	else {
		return CameraData(this->perspectiveData, this->ViewMatrix());
	}
}

void Camera::DrawImGui() {
	if (ImGui::TreeNode("LayerMask")) {
		const float size = ImGui::CalcTextSize("00").x;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 8; x++) {
				if (x > 0) {
					ImGui::SameLine();
				}

				uint8_t layer = y * 8 + x;

				ImGui::PushID(layer);

				if (ImGui::Selectable(
					std::to_string(layer).c_str(),
					this->layerMask.Test(layer),
					0,
					ImVec2(size, size)
				)) {
					this->layerMask = this->layerMask.value ^ (1 << layer);
				}

				ImGui::PopID();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Render Passes")) {
		bool rendersColor = this->HasPass(RenderPassType::Color);
		bool rendersDepthPrepass = this->HasPass(RenderPassType::DepthPrepass);
		bool rendersGizmos = this->HasPass(RenderPassType::Gizmos);
		bool rendersPostProcessing = this->HasPass(RenderPassType::PostProcessing);
		bool rendersTransparent = this->HasPass(RenderPassType::Transparent);
		bool rendersAdditive = this->HasPass(RenderPassType::Additive);
		bool rendersVolumetric = this->HasPass(RenderPassType::Volumetric);
		bool rendersSSAO = this->HasPass(RenderPassType::SSAO);
		bool rendersMask = this->HasPass(RenderPassType::Mask);
		bool rendersUI = this->HasPass(RenderPassType::UI);

		ImGui::Checkbox("Render Color", &rendersColor);
		ImGui::Checkbox("Render DepthPrepass", &rendersDepthPrepass);
		ImGui::Checkbox("Render Gizmos", &rendersGizmos);
		ImGui::Checkbox("Render PostProcessing", &rendersPostProcessing);
		ImGui::Checkbox("Render Transparent", &rendersTransparent);
		ImGui::Checkbox("Render Additive", &rendersAdditive);
		ImGui::Checkbox("Render Volumetric", &rendersVolumetric);
		ImGui::Checkbox("Render SSAO", &rendersSSAO);
		ImGui::Checkbox("Render Mask", &rendersMask);
		ImGui::Checkbox("Render UI", &rendersUI);

		SetPasses((RenderPassType) 0);

		if (rendersColor) {
			AddPass(RenderPassType::Color);
		}
		if (rendersDepthPrepass) {
			AddPass(RenderPassType::DepthPrepass);
		}
		if (rendersGizmos) {
			AddPass(RenderPassType::Gizmos);
		}
		if (rendersPostProcessing) {
			AddPass(RenderPassType::PostProcessing);
		}
		if (rendersTransparent) {
			AddPass(RenderPassType::Transparent);
		}
		if (rendersAdditive) {
			AddPass(RenderPassType::Additive);
		}
		if (rendersVolumetric) {
			AddPass(RenderPassType::Volumetric);
		}
		if (rendersSSAO) {
			AddPass(RenderPassType::SSAO);
		}
		if (rendersMask) {
			AddPass(RenderPassType::Mask);
		}
		if (rendersUI) {
			AddPass(RenderPassType::UI);
		}

		ImGui::TreePop();
	}

	const char* cameraTypes[] = { "Orthographic", "Perspective" };
	int currentCameraType = this->type == CameraType::Orthographic ? 0 : 1;

	ImGui::Combo("Camera Type", &currentCameraType, cameraTypes, 2);

	if (currentCameraType == 0) {
		this->SetType(CameraType::Orthographic);
	}
	else {
		this->SetType(CameraType::Perspective);
	}

	if (this->type == CameraType::Orthographic) {
		float size = this->orthoData.top - this->orthoData.bottom;
		float orthoAspec = (this->orthoData.left - this->orthoData.right) / size;

		ImGui::InputFloat("Orthographic Size", &size);
		ImGui::InputFloat("Aspect Ratio", &orthoAspec);

		this->orthoData.top = size / 2;
		this->orthoData.bottom = -size / 2;

		this->orthoData.left = (size / 2) * orthoAspec;
		this->orthoData.right = -(size / 2) * orthoAspec;

		ImGui::InputFloat("Near Plane", &this->orthoData.znear);
		SetNearPlane(this->orthoData.znear);

		ImGui::InputFloat("Far Plane", &this->orthoData.zfar);
		SetFarPlane(this->orthoData.zfar);
	}
	else {
		ImGui::InputFloat("FoV", &this->perspectiveData.fovyDegrees);
		ImGui::InputFloat("Aspect Ratio", &this->perspectiveData.aspectRatio);

		ImGui::InputFloat("Near Plane", &this->perspectiveData.nearPlane);
		SetNearPlane(this->perspectiveData.nearPlane);

		ImGui::InputFloat("Far Plane", &this->perspectiveData.farPlane);
		SetFarPlane(this->perspectiveData.farPlane);
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
			this->orthoParams.top,
      this->orthoParams.znear,
      this->orthoParams.zfar
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
