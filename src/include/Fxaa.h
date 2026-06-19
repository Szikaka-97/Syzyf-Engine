#pragma once

#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderDispatch;

class Fxaa : public PostProcessEffect, public ImGuiDrawable
{
private:
  ComputeShaderDispatch* fxaaShader = nullptr;

  serialized float edgeThreshold = 1.0f / 8.0f;
  serialized float edgeThresholdMin = 1.0f / 24.0f;
  serialized float subpixCap = 0.75f;
  serialized float subpixTrim = 0.25f;

  serialized float searchThreshold = 1.0f / 4.0f;
  serialized float searchSteps = 32.0f;

public:
  bool debugEdges = false;

  Fxaa();

  virtual void OnPostProcess(const PostProcessParams* params) override;
  virtual void DrawImGui() override;

	virtual int Order() const override;
};
