#pragma once
#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderProgram;

class MaskEffects : public PostProcessEffect, public ImGuiDrawable {
public:
    glm::vec4 xrayColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.8f);
    float xrayIntensity = 1.0f;
    glm::vec4 outlineColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    float outlineIntensity = 1.0f;
private:
    ComputeShaderProgram* shader;
public:
    MaskEffects();
    virtual void OnPostProcess(const PostProcessParams* params) override;
    virtual void DrawImGui() override;
};
