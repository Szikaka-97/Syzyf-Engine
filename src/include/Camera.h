#pragma once

#include <glm/glm.hpp>

#include <GameObject.h>
#include <Layer.h>
#include <Debug.h>

struct CameraData;

class Viewport;

class Camera : public GameObject, public ImGuiDrawable {
public:
	struct Perspective {
		Perspective() = default;
		Perspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane);

		serialized float fovyDegrees = 0;
		serialized float aspectRatio = 0;
		serialized float nearPlane = 0;
		serialized float farPlane = 0;
	};
	struct Orthographic {
		Orthographic() = default;
		Orthographic(float left, float right, float top, float bottom, float nearPlane, float farPlane);
		Orthographic(glm::vec2 viewportSize);

		serialized float left = 0;
		serialized float right = 0;
		serialized float top = 0;
		serialized float bottom = 0;
		serialized float znear = 0;
		serialized float zfar = 0;
	};

	enum class CameraType {
		Perspective,
		Orthographic
	};
private:
	serialized CameraType type;

	serialized Perspective perspectiveData;
	serialized Orthographic orthoData;
	Viewport* renderTarget;
	serialized LayerMask layerMask;
	serialized int priority;
public:
	Camera();
	Camera(Perspective perspectiveData);
	Camera(Orthographic orthoData);
	virtual ~Camera();

	void MakePerspective();
	void MakePerspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane);

	void MakeOrtho();
	void MakeOrtho(float left, float right, float top, float bottom, float znear, float zfar);

	CameraType GetType() const;
	void SetType(CameraType type);

	float GetFov() const;
	float GetFovRad() const;
	float GetAspectRatio() const;
	float GetNearPlane() const;
	float GetFarPlane() const;
	
	void SetFov(float newFov);
	void SetFovRad(float newFovRad);
	void SetAspectRatio(float newAspectRatio);
	void SetNearPlane(float newNearPlane);
	void SetFarPlane(float newFarPlane);

	float GetLeftOrthoPlane() const;
	float GetRightOrthoPlane() const;
	float GetTopOrthoPlane() const;
	float GetBottomOrthoPlane() const;

	void SetLeftOrthoPlane(float newLeft);
	void SetRightOrthoPlane(float newRight);
	void SetTopOrthoPlane(float newTop);
	void SetBottomOrthoPlane(float newBottom);

	glm::mat4 ViewMatrix() const;
	glm::mat4 ProjectionMatrix() const;
	glm::mat4 ViewProjectionMatrix() const;

	Viewport* GetRenderTarget() const;
	void SetRenderTarget(Viewport* viewport);

	uint32_t GetLayerMask() const;
	bool TestLayer(uint8_t layer);

	int GetPriority() const;
	void SetPriority(int priority);

	void SetLayerMask(LayerMask newMask);
	void AddLayerToMask(uint8_t layer);
	void RemoveLayerFromMask(uint8_t layer);

	void SetAsMainCamera();

	CameraData GetCameraData() const;

	virtual void DrawImGui();
};

struct CameraData {
	union {
		const Camera::Orthographic orthoParams;
		const Camera::Perspective perspectiveParams;
	};
	const Camera::CameraType type;
	const glm::mat4 cameraTransform;

	CameraData(const Camera::Orthographic& orthoParams, const glm::mat4& cameraTransform);
	CameraData(const Camera::Perspective& perspectiveParams, const glm::mat4& cameraTransform);

	glm::mat4 ViewMatrix() const;
	glm::mat4 ProjectionMatrix() const;
	glm::mat4 ViewProjectionMatrix() const;

	float GetFov() const;
	float GetFovRad() const;
	float GetAspectRatio() const;
	float GetNearPlane() const;
	float GetFarPlane() const;
};
