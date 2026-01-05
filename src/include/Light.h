#pragma once

#include <GameObject.h>
#include <Material.h>

#include "../res/shaders/shared/shared.h"

#define LIGHTS_DRAW_GIZMOS true

class Light : public GameObject {
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
		float attenuation;

		PointLight(const glm::vec3& color, float range, float intensity, float attenuation = 1.0f);
	};

	struct SpotLight {
		glm::vec3 color;
		float spotlightAngle;
		float range;
		float intensity;
		float attenuation;

		SpotLight(const glm::vec3& color, float spotlightAngle, float range, float intensity, float attenuation = 1.0f);
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
	float attenuation;

	bool shadowCasting;

	mutable glm::mat4 savedTransform;

#if LIGHTS_DRAW_GIZMOS
	Material* gizmoMat;
#endif
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
	float GetAttenuation() const;
	bool IsShadowCasting() const;

	void SetType(LightType type);
	void SetColor(const glm::vec3& color);
	void SetRange(float range);
	void SetIntensity(float intensity);
	void SetAttenuation(float attenuation);
	void SetShadowCasting(bool shadowCasting);

	bool IsDirty() const;

#if LIGHTS_DRAW_GIZMOS
	void Render();
#endif

	ShaderLightRep GetShaderRepresentation() const;
};