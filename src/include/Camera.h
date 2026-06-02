#pragma once

#include <glm/glm.hpp>

#include <GameObject.h>
#include <Layer.h>
#include <Debug.h>

struct CameraData;

class Viewport;

enum class RenderPassType {
	Color = 1,
	DepthPrepass = 2,
	Shadows = 6,
	Gizmos = 8,
	PostProcessing = 16,
	Transparent = 32,
	Additive = 64,
	Volumetric = 128,
    SSAO = 256,
    Mask = 512,
    UI = 1024,
};

class Camera : public GameObject, public ImGuiDrawable {
public:
	struct Perspective {
		Perspective() = default;
		Perspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane);

		float fovyDegrees = 0;
		float aspectRatio = 0;
		float nearPlane = 0;
		float farPlane = 0;
	};
	struct Orthographic {
		Orthographic() = default;
		Orthographic(float left, float right, float top, float bottom, float znear, float zfar);
		Orthographic(glm::vec2 viewportSize);

		float left = 0;
		float right = 0;
		float top = 0;
		float bottom = 0;
		float znear = 0;
		float zfar = 0;
	};

	enum class CameraType {
		Perspective,
		Orthographic
	};
private:
	CameraType type;

	Perspective perspectiveData;
	Orthographic orthoData;
	Viewport* renderTarget;
	LayerMask layerMask;
	RenderPassType passes;
	int priority;
public:
	static RenderPassType DefaultCameraPasses;
	static RenderPassType DefaultMainCameraPasses;

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

	RenderPassType GetPasses() const;
	void SetPasses(RenderPassType passes);
	bool HasPass(RenderPassType pass);
	void AddPass(RenderPassType pass);
	void RemovePass(RenderPassType pass);

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

inline constexpr RenderPassType operator&(RenderPassType a, RenderPassType b) {
	return static_cast<RenderPassType>(static_cast<int>(a) & static_cast<int>(b));
}

inline constexpr RenderPassType operator|(RenderPassType a, RenderPassType b) {
	return static_cast<RenderPassType>(static_cast<int>(a) | static_cast<int>(b));
}

inline constexpr RenderPassType& operator|=(RenderPassType& a, RenderPassType b) {
	a = a | b;

	return a;
}

inline constexpr RenderPassType& operator&=(RenderPassType& a, RenderPassType b) {
	a = a & b;

	return a;
}

inline constexpr RenderPassType operator~(RenderPassType& a) {
	a = static_cast<RenderPassType>(~static_cast<int>(a));

	return a;
}