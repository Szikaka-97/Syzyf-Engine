#pragma once

#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderDispatch;

// Must run after the tonemapper
class ColorGrading : public PostProcessEffect, public ImGuiDrawable {
public:
    float brightness = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;

    float chromaticAberrationStrength = 0.0f;

    float filmGrainStrength = 0.0f;
    float vignetteStrength = 0.0f;
private:
	ComputeShaderDispatch* colorGradingShader;
    Texture2D* curveTexture = nullptr;

    glm::vec3 chromaticAberrationOffsets { 0.009f, 0.006f, -0.006f };
public:
	ColorGrading();

    virtual void OnPostProcess(const PostProcessParams* params) override;

    void SetCurveTexture(Texture2D* texture);

    virtual void DrawImGui() override;

	virtual int Order() const override;
};
