#pragma once

#include <GameObject.h>

class Light : public GameObject {
public:
	enum class LightType {
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	struct LightRep {
		glm::vec3 position;
		unsigned int type;

		glm::vec3 direction;
		float range;

		glm::vec3 color;
		float spotlightAngle;

		float intensity;
		float attenuation;
		uint enabled;
		uint _padding;
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
	glm::vec3 color;
	
	float range;
	float spotlightAngle;
	float intensity;
	float attenuation;
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

	LightRep GetShaderRepresentation() const;
};