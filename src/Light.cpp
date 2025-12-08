#include <Light.h>

Light::PointLight::PointLight(const glm::vec3& color, float range, float intensity, float attenuation):
color(color),
range(range),
intensity(intensity),
attenuation(attenuation) { }

Light::SpotLight::SpotLight(const glm::vec3& color, float spotlightAngle, float range, float intensity, float attenuation):
color(color),
spotlightAngle(spotlightAngle),
range(range),
intensity(intensity),
attenuation(attenuation) { }

Light::DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity):
color(color),
intensity(intensity) { }

Light::Light(Light::PointLight lightInfo):
type(Light::LightType::Point),
dirty(true),
color(lightInfo.color),
range(lightInfo.range),
spotlightAngle(0),
intensity(lightInfo.intensity),
attenuation(lightInfo.attenuation),
savedTransform(GlobalTransform()) { }

Light::Light(Light::SpotLight lightInfo):
type(Light::LightType::Spot),
dirty(true),
color(lightInfo.color),
range(lightInfo.range),
spotlightAngle(lightInfo.spotlightAngle),
intensity(lightInfo.intensity),
attenuation(lightInfo.attenuation),
savedTransform(GlobalTransform()) { }

Light::Light(Light::DirectionalLight lightInfo):
type(Light::LightType::Directional),
dirty(true),
color(lightInfo.color),
range(0),
spotlightAngle(0),
intensity(lightInfo.intensity),
attenuation(0),
savedTransform(GlobalTransform()) { }

void Light::Set(Light::PointLight lightInfo) {
	this->type = Light::LightType::Point;
	this->color = lightInfo.color;
	this->range = lightInfo.range;
	this->intensity = lightInfo.intensity;
	this->attenuation = lightInfo.attenuation;

	this->dirty = true;
}
void Light::Set(Light::SpotLight lightInfo) {
	this->type = Light::LightType::Spot;
	this->color = lightInfo.color;
	this->range = lightInfo.range;
	this->spotlightAngle = lightInfo.spotlightAngle;
	this->intensity = lightInfo.intensity;
	this->attenuation = lightInfo.attenuation;

	this->dirty = true;
}
void Light::Set(Light::DirectionalLight lightInfo) {
	this->type = Light::LightType::Directional;
	this->color = lightInfo.color;
	this->intensity = lightInfo.intensity;

	this->dirty = true;
}

Light::LightType Light::GetType() const {
	return this->type;
}

glm::vec3 Light::GetColor() const {
	return this->color;
}

float Light::GetRange() const {
	return this->range;
}

float Light::GetSpotlightAngle() const {
	return this->spotlightAngle;
}

float Light::GetIntensity() const {
	return this->intensity;
}

float Light::GetAttenuation() const {
	return this->attenuation;
}

void Light::SetType(Light::LightType type) {
	this->type = type;

	this->dirty = true;
}

void Light::SetColor(const glm::vec3& color) {
	this->color = color;

	this->dirty = true;
}

void Light::SetRange(float range) {
	this->range = range;

	this->dirty = true;
}

void Light::SetIntensity(float intensity) {
	this->intensity = intensity;

	this->dirty = true;
}

void Light::SetAttenuation(float attenuation) {
	this->attenuation = attenuation;

	this->dirty = true;
}

bool Light::IsDirty() const {
	return this->dirty || (this->savedTransform != GlobalTransform().Value());
}

ShaderLightRep Light::GetShaderRepresentation() const {
	ShaderLightRep result;
	
	this->savedTransform = GlobalTransform();
	this->dirty = false;

	result.position = this->GlobalTransform().Position();
	result.type = (unsigned int) this->type;
	result.direction = this->GlobalTransform().Forward();
	result.range = this->range;
	result.color = this->color;
	result.spotlightAngle = this->spotlightAngle;
	result.intensity = this->intensity;
	result.attenuation = this->attenuation;
	result.enabled = this->IsEnabled();

	return result;
}