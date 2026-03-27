#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Material;

class Fog : public PostProcessEffect, public ImGuiDrawable {
private:
  ShaderProgram* shader;
  Material* material;

  float minDistance;
  float maxDistance;
  glm::vec4 fogColor;
public:
  Fog(float minDistance = 0.1, float maxDistance = 3.0, glm::vec4 fogColor = glm::vec4(0.4));

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
