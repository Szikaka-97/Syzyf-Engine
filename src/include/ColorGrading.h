#pragma once

#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderDispatch;

// Must run after the tonemapper
class ColorGrading : public PostProcessEffect, public ImGuiDrawable {
private:
	ComputeShaderDispatch* colorGradingShader;

    float brightness;
    float contrast;
    float saturation;

    float gamma;
    bool enableGammaCorrection;

    Texture2D* curveTexture;
public:
	ColorGrading();

    virtual void OnPostProcess(const PostProcessParams* params) override;

    void SetBrightness(float brightness);
    void SetContrast(float contrast);
    void SetSaturation(float saturation);
    void SetCurveTexture(Texture2D* texture);

    void SetGamma(float gamma);
    void SetEnableGammaCorrection(bool enableGammaCorrection);

    virtual void DrawImGui() override;
};
