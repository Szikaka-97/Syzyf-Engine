#include <Light.h>

#include <Resources.h>
#include <Material.h>
#include <Mesh.h>
#include <Graphics.h>
#include <imgui.h>

ShaderProgram* GetGizmoShader() {
	static ShaderProgram* gizmoProg = ShaderProgram::Build()
	.WithVertexShader(Resources::Get<VertexShader>("./res/shaders/lit.vert"))
	.WithPixelShader(Resources::Get<PixelShader>("./res/shaders/halo.frag"))
	.Link();
	
	gizmoProg->SetCastsShadows(false);
	gizmoProg->SetIgnoresDepthPrepass(true);

	return gizmoProg;
}

Light::PointLight::PointLight(const glm::vec3& color, float range, float intensity, float linearAttenuation, float quadraticAttenuation):
color(color),
range(range),
intensity(intensity),
linearAttenuation(linearAttenuation),
quadraticAttenuation(quadraticAttenuation) { }

Light::SpotLight::SpotLight(const glm::vec3& color, float spotlightAngle, float range, float intensity, float linearAttenuation, float quadraticAttenuation):
color(color),
spotlightAngle(spotlightAngle),
range(range),
intensity(intensity),
linearAttenuation(linearAttenuation),
quadraticAttenuation(quadraticAttenuation) { }

Light::DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity):
color(color),
intensity(intensity) { }

Light::~Light() { }

Light::Light(Light::PointLight lightInfo):
type(Light::LightType::Point),
dirty(true),
color(lightInfo.color),
range(lightInfo.range),
spotlightAngle(0),
intensity(lightInfo.intensity),
linearAttenuation(lightInfo.linearAttenuation),
quadraticAttenuation(lightInfo.quadraticAttenuation),
shadowCasting(false),
savedTransform(GlobalTransform()) {
	this->gizmoMat = new Material(GetGizmoShader());
}

Light::Light(Light::SpotLight lightInfo):
type(Light::LightType::Spot),
dirty(true),
color(lightInfo.color),
range(lightInfo.range),
spotlightAngle(lightInfo.spotlightAngle),
intensity(lightInfo.intensity),
linearAttenuation(lightInfo.linearAttenuation),
quadraticAttenuation(lightInfo.quadraticAttenuation),
shadowCasting(false),
savedTransform(GlobalTransform()) {
	this->gizmoMat = new Material(GetGizmoShader());
}

Light::Light(Light::DirectionalLight lightInfo):
type(Light::LightType::Directional),
dirty(true),
color(lightInfo.color),
range(0),
spotlightAngle(0),
intensity(lightInfo.intensity),
linearAttenuation(0),
quadraticAttenuation(0),
shadowCasting(false),
savedTransform(GlobalTransform()) {
	this->gizmoMat = new Material(GetGizmoShader());
}

void Light::Set(Light::PointLight lightInfo) {
	this->type = Light::LightType::Point;
	this->color = lightInfo.color;
	this->range = lightInfo.range;
	this->intensity = lightInfo.intensity;
	this->linearAttenuation = lightInfo.linearAttenuation;
	this->quadraticAttenuation = lightInfo.quadraticAttenuation;

	this->dirty = true;
}
void Light::Set(Light::SpotLight lightInfo) {
	this->type = Light::LightType::Spot;
	this->color = lightInfo.color;
	this->range = lightInfo.range;
	this->spotlightAngle = lightInfo.spotlightAngle;
	this->intensity = lightInfo.intensity;
	this->linearAttenuation = lightInfo.linearAttenuation;
	this->quadraticAttenuation = lightInfo.quadraticAttenuation;

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

float Light::GetLinearAttenuation() const {
	return this->linearAttenuation;
}

float Light::GetQuadraticAttenuation() const {
	return this->quadraticAttenuation;
}

bool Light::IsShadowCasting() const {
	return this->shadowCasting;
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

void Light::SetSpotlightAngle(float angle) {
	this->spotlightAngle = angle;

	this->dirty = true;
}

void Light::SetIntensity(float intensity) {
	this->intensity = intensity;

	this->dirty = true;
}

void Light::SetLinearAttenuation(float attenuation) {
	this->linearAttenuation = attenuation;

	this->dirty = true;
}

void Light::SetQuadraticAttenuation(float attenuation) {
	this->quadraticAttenuation = attenuation;

	this->dirty = true;
}

void Light::SetShadowCasting(bool shadowCasting) {
	this->shadowCasting = shadowCasting;
	
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
	result.intensity = this->IsEnabled() ? this->intensity : 0;
	result.linearAttenuation = this->linearAttenuation;
	result.quadraticAttenuation = this->quadraticAttenuation;

	return result;
}

void Light::DrawGizmos() {
	static Mesh* directionalGizmoMesh = Resources::Get<Mesh>("./res/models/directional_gizmo.obj");
	static Mesh* spotGizmoMesh = Resources::Get<Mesh>("./res/models/spot_gizmo.obj");
	static Mesh* pointGizmoMesh = Resources::Get<Mesh>("./res/models/point_gizmo.obj");

	this->gizmoMat->SetValue("uColor", this->color * (this->intensity * 5));

	if (this->type == LightType::Directional) {
		GetScene()->GetGraphics()->DrawGizmoMesh(directionalGizmoMesh, 0, this->gizmoMat, GlobalTransform());
	}
	else if (this->type == LightType::Spot) {
		GetScene()->GetGraphics()->DrawGizmoMesh(spotGizmoMesh, 0, this->gizmoMat, GlobalTransform());
	}
	else {
		GetScene()->GetGraphics()->DrawGizmoMesh(pointGizmoMesh, 0, this->gizmoMat, GlobalTransform());
	}
}

void Light::DrawImGui() {
	{
		const char* types[] { "Point Light", "Spot Light", "Directional Light" };

		int currentType = (int) this->type;

		ImGui::Combo("Light Type", &currentType, types, 3);

		if (currentType != (int) this->type) {
			SetType((LightType) currentType);
		}
	}
	{
		glm::vec3 newColor = this->color;
	
		ImGui::ColorEdit3("Color", &newColor[0]);
	
		if (newColor != this->color) {
			SetColor(newColor);
		}
	}
	if (this->type != LightType::Directional) {
		float newRange = this->range;
	
		ImGui::InputFloat("Range", &newRange);
	
		if (newRange != this->range) {
			SetRange(newRange);
		}
	}
	if (this->type == LightType::Spot) {
		float newAngle = glm::degrees(this->spotlightAngle);
	
		ImGui::InputFloat("Spotlight Angle", &newAngle);
	
		if (newAngle != glm::degrees(this->spotlightAngle)) {
			SetSpotlightAngle(glm::radians(newAngle));
		}
	}
	{
		float newIntensity = this->intensity;
	
		ImGui::InputFloat("Intensity", &newIntensity);
	
		if (newIntensity != this->intensity) {
			SetIntensity(newIntensity);
		}
	}
	if (this->type != LightType::Directional) {
		float newLinearAttenuation = this->linearAttenuation;
	
		ImGui::InputFloat("Linear Attenuation", &newLinearAttenuation);
	
		if (newLinearAttenuation != this->linearAttenuation) {
			SetLinearAttenuation(newLinearAttenuation);
		}
	}
	if (this->type != LightType::Directional) {
		float newQuadraticAttenuation = this->quadraticAttenuation;
	
		ImGui::InputFloat("Quadratic Attenuation", &newQuadraticAttenuation);
	
		if (newQuadraticAttenuation != this->quadraticAttenuation) {
			SetQuadraticAttenuation(newQuadraticAttenuation);
		}
	}
	{
		bool newShadowCasting = this->shadowCasting;
	
		ImGui::Checkbox("Casts Shadows", &newShadowCasting);
	
		if (newShadowCasting != this->shadowCasting) {
			SetShadowCasting(newShadowCasting);
		}
	}
}