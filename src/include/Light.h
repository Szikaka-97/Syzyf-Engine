#pragma once

#include <GameObject.h>
#include <Material.h>
#include <Debug.h>

#include "../res/shaders/shared/shared.h"

class Light : public GameObject, public ImGuiDrawable {
public:
	enum class LightType {
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	struct PointLight {
		glm::vec3 color;
		float range;
		float intensity;
		float linearAttenuation;
		float quadraticAttenuation;

		PointLight(const glm::vec3& color, float range, float intensity, float linearAttenuation = 1.0f, float quadraticAttenuation = 1.0f);
	};

	struct SpotLight {
		glm::vec3 color;
		float spotlightAngle;
		float range;
		float intensity;
		float linearAttenuation;
		float quadraticAttenuation;

		SpotLight(const glm::vec3& color, float spotlightAngle, float range, float intensity, float linearAttenuation = 1.0f, float quadraticAttenuation = 1.0f);
	};

	struct DirectionalLight {
		glm::vec3 color;
		float intensity;

		DirectionalLight(const glm::vec3& color, float intensity);
	};
private:
	LightType type;
	mutable bool dirty;
	glm::vec3 color;
	
	float range;
	float spotlightAngle;
	float intensity;
	float linearAttenuation;
	float quadraticAttenuation;

	bool shadowCasting;

	mutable glm::mat4 savedTransform;

	Material* gizmoMat;
public:
	virtual ~Light();

	Light(PointLight lightInfo);
	Light(SpotLight lightInfo);
	Light(DirectionalLight lightInfo);

	void Set(PointLight lightInfo);
	void Set(SpotLight lightInfo);
	void Set(DirectionalLight lightInfo);

	LightType GetType() const;
	glm::vec3 GetColor() const;
	float GetRange() const;
	float GetSpotlightAngle() const;
	float GetIntensity() const;
	float GetLinearAttenuation() const;
	float GetQuadraticAttenuation() const;
	bool IsShadowCasting() const;

	void SetType(LightType type);
	void SetColor(const glm::vec3& color);
	void SetRange(float range);
	void SetSpotlightAngle(float angle);
	void SetIntensity(float intensity);
	void SetLinearAttenuation(float attenuation);
	void SetQuadraticAttenuation(float attenuation);
	void SetShadowCasting(bool shadowCasting);

	bool IsDirty() const;

	void DrawGizmos();

	virtual void DrawImGui();
	
	ShaderLightRep GetShaderRepresentation() const;
};