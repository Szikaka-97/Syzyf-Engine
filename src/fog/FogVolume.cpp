#include <fog/FogVolume.h>
#include <Graphics.h>
#include <Material.h>
#include <Mesh.h>
#include <Resources.h>
#include <Scene.h>
#include <Shader.h>
#include <imgui.h>
#include <LightSystem.h>

#include "Texture.h"

FogVolume::FogVolume() {
  this->mesh = GetScene()->Resources()->Get<Mesh>("./res/models/not_cube.obj");

  ShaderProgram *prog = ShaderProgram::Build()
  .WithVertexShader("./res/shaders/fog/fog_volume.vert")
  .WithPixelShader("./res/shaders/fog/fog_volume.frag")
  .Link();

  this->material = new Material(prog);
}

void FogVolume::Render() {
  this->material->SetValue("stepSize", this->stepSize);
  this->material->SetValue("scatteringDensity", this->scatteringDensity);
  this->material->SetValue("absorptionDensity", this->absorptionDensity);
  this->material->SetValue("scatteringColor", this->scatteringColor);
  this->material->SetValue("emissiveStrength", this->emissiveStrength);
  this->material->SetValue("k", this->k);
  this->material->SetValue("transmittanceThreshold",
                           this->transmittanceThreshold);
  this->material->SetValue("coverage", this->coverage);
  this->material->SetValue("sharpness", this->sharpness);
  this->material->SetValue("bias", this->bias);
  if (this->noiseTexture != nullptr) {
      this->material->SetValue("useNoiseTex", true);
      this->material->SetValue("noiseTex", this->noiseTexture);
      this->material->SetValue("noiseScale", this->noiseScale);
      this->material->SetValue("windDirection", this->windDirection);
  } else {
    this->material->SetValue("useNoiseTex", false);
  }
  if (this->colorRamp != nullptr) {
      this->material->SetValue("useColorRamp", true);
      this->material->SetValue("colorRamp", this->colorRamp);
  } else {
      this->material->SetValue("useColorRamp", false);
  }

  std::vector<int> intersectingLightIndices;
  auto* lights = GetScene()->GetGraphics()->GetLightSystem()->GetAllObjects();

  glm::vec3 scale = this->GlobalTransform().Scale();
  glm::vec3 position = this->GlobalTransform().Position();
  glm::vec3 boxMin = position - (scale * 0.5f);
  glm::vec3 boxMax = position + (scale * 0.5f);

  int lightIndex = 0;
  for (const auto& light : *lights) {
    if (!light->IsEnabled()) {
      lightIndex++;
      continue;
    }

    if (light->GetType() == Light::LightType::Directional) {
      intersectingLightIndices.push_back(lightIndex);
    } else if (light->GetType() == Light::LightType::Point || light->GetType() == Light::LightType::Spot) {
      glm::vec3 lightPos = light->GlobalTransform().Position();
      float range = light->GetRange();

      glm::vec3 closestPoint = glm::clamp(lightPos, boxMin, boxMax);
      glm::vec3 difference = lightPos - closestPoint;
      float distanceSquared = glm::dot(difference, difference);

      if (distanceSquared <= (range * range)) {
        intersectingLightIndices.push_back(lightIndex);
      }
    }

    lightIndex++;
  }

  this->material->Bind();
  GLuint shaderHandle = this->material->GetShader()->GetHandle();
  int countLocation = glGetUniformLocation(shaderHandle, "intersectingLightCount");
  glUniform1i(countLocation, intersectingLightIndices.size());

  if (!intersectingLightIndices.empty()) {
    int arrayLocation = glGetUniformLocation(shaderHandle, "intersectingLightIndices");
    glUniform1iv(arrayLocation, intersectingLightIndices.size(), intersectingLightIndices.data());
  }

  GetScene()->GetGraphics()->DrawMesh(this->mesh, 0, this->material,
                                      this->GlobalTransform());
}

void FogVolume::DrawImGui() {
  ImGui::Text("Fog Volume Settings");
  ImGui::SliderFloat("Step Size", &this->stepSize, 0.001f, 0.5f);
  ImGui::SliderFloat("Scattering Density", &this->scatteringDensity, 0.0f,
                     2.0f);
  ImGui::SliderFloat("Absorption Density", &this->absorptionDensity, 0.0f,
                     2.0f);
  ImGui::ColorEdit3("Scattering Color", &this->scatteringColor.x);
  ImGui::SliderFloat("Emissive Strength", &this->emissiveStrength, 0.0, 5.0f);
  ImGui::SliderFloat("Anisotropy", &this->k, -0.99f, 0.99f);
  ImGui::InputFloat("Transmittance Threshold", &this->transmittanceThreshold);
  ImGui::InputFloat("Bias", &this->bias, -0.99f, 0.99f);
  ImGui::InputFloat("Noise Scale", &this->noiseScale, 0.0f, 5.0f);
  ImGui::InputFloat3("Wind Direction", &this->windDirection.x);

  ImGui::InputFloat("Coverage", &this->coverage, -0.99f, 0.99f);
  ImGui::InputFloat("Sharpness", &this->sharpness, -0.99f, 0.99f);
}
