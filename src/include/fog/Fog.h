#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Material;

class Fog : public PostProcessEffect, public ImGuiDrawable {
public:
    enum class Type {
        Linear = 0,
        Exp = 1,
        Exp2 = 2,
    };

  Type fogType;
  float minDistance;
  float maxDistance;
  float density;
  glm::vec4 fogColor;
private:
  ShaderProgram* shader;
  Material* material;
public:
  Fog(Type fogType = Type::Linear, float minDistance = 0.1, float maxDistance = 100.0, glm::vec4 fogColor = glm::vec4(0.4));

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
