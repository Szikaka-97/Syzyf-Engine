#pragma once
#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderProgram;

class MaskEffects : public PostProcessEffect, public ImGuiDrawable {
public:
    glm::vec3 xrayColor = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 outlineColor = glm::vec3(1.0f, 0.5f, 0.0f);
private:
    ComputeShaderProgram* shader;
public:
    MaskEffects();
    virtual void OnPostProcess(const PostProcessParams* params) override;
    virtual void DrawImGui() override;
};
