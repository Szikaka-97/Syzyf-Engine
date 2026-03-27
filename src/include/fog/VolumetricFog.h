#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Material;

class VolumetricFog : public PostProcessEffect, public ImGuiDrawable {
private:
  ShaderProgram* shader;
  Material* material;

  float stepSize = 0.8f;
  float rayZFar = 50.0f;
  float scatteringDensity = 0.01f;
  float absorptionDensity = 0.03f;
  glm::vec3 scatteringColor = glm::vec3(1.0f);
  Variable k to adjust scattering direction for the Phasing function, where:
  float k = 0.5f;

public:
  VolumetricFog(float stepSize = 0.8f, float rayZFar = 50.0f, float scatteringDensity = 0.01f, float absorptionDensity = 0.03f, glm::vec3 scatteringColor = glm::vec3(1.0f), float k = 0.5f);

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
