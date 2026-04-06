#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Material;

class VolumetricFog : public PostProcessEffect, public ImGuiDrawable {
private:
  ShaderProgram* shader;
  Material* material;

  float stepSize;
  float rayZFar;
  float scatteringDensity;
  float absorptionDensity;
  glm::vec3 scatteringColor;
  // https://lonvanettinger.com/portfolio-pages/fog-ray-march-article-1
  // Variable k to adjust scattering direction for the Phasing function
  //  k = 0 isotropic, k > 0 forward scattering (like fog or dust), k < 0 backward scattering
  float k;
  float transmittanceThreshold;

public:
  VolumetricFog(
    float stepSize = 0.8f,
    float rayZFar = 50.0f,
    float scatteringDensity = 0.01f,
    float absorptionDensity = 0.03f,
    glm::vec3 scatteringColor = glm::vec3(1.0f),
    float k = 0.5f,
    float transmittanceThreshold = 0.001f
  );

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
