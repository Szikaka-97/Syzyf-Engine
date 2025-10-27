#pragma once

#include <GameObject.h>
#include <glm/glm.hpp>

class Camera : public GameObject {
public:
	struct Perspective {
		float fovyDegrees = 0;
		float aspectRatio = 0;
		float nearPlane = 0;
		float farPlane = 0;
	};
	struct Orthographic {
		float left = 0;
		float right = 0;
		float top = 0;
		float bottom = 0;
	};

	enum class CameraType {
		Perspective,
		Orthographic
	};
private:
	CameraType type;

	Perspective perspectiveData;
	Orthographic orthoData;

	static Camera* mainCamera;
public:
	Camera(Perspective perspectiveData);
	Camera(Orthographic orthoData);

	void MakePerspective();
	void MakePerspective(float fovyDegrees, float aspectRatio, float nearPlane, float farPlane);

	void MakeOrtho();
	void MakeOrtho(float left, float right, float top, float bottom);

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

	static Camera* GetMainCamera();
	void SetAsMainCamera();
};