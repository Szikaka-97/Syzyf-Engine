#pragma once

#include <GameObject.h>

#include "../res/shaders/shared/shared.h"

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

	mutable glm::mat4 savedTransform;
public:
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

	void SetType(LightType type);
	void SetColor(const glm::vec3& color);
	void SetRange(float range);
	void SetIntensity(float intensity);
	void SetAttenuation(float attenuation);

	bool IsDirty() const;

	ShaderLightRep GetShaderRepresentation() const;
};