#pragma once

#include "Debug.h"
#include "PostProcessEffect.h"
#include "Shader.h"

class Fog : public PostProcessEffect, public ImGuiDrawable {
private:
  ShaderProgram* shader;
public:
  Fog();

  virtual void OnPostProcess(const PostProcessParams* params) override;

  virtual void DrawImGui() override;
};
