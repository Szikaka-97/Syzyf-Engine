#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Material;

class Fog : public PostProcessEffect, public ImGuiDrawable {
private:
  std::unique_ptr<ShaderProgram> shader;
  std::unique_ptr<Material> material;

  float near;
  float far;
  float minDistance;
  float maxDistance;
  glm::vec4 fogColor;
public:
  Fog(float near = 0.1, float far = 100.0, float minDistance = 0.1, float maxDistance = 3.0, glm::vec4 fogColor = glm::vec4(0.4));

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
