#pragma once
#include <PostProcessEffect.h>
#include <Debug.h>

class ComputeShaderProgram;

class MaskEffects : public PostProcessEffect, public ImGuiDrawable {
public:
    serialized glm::vec4 xrayColor = glm::vec4(1.0f, 0.0f, 1.0f, 0.2f);
    serialized float xrayIntensity = 1.0f;
    serialized glm::vec4 outlineColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    serialized float outlineIntensity = 1.0f;
private:
    ComputeShaderProgram* shader;
public:
    MaskEffects();
    virtual void OnPostProcess(const PostProcessParams* params) override;
    virtual void DrawImGui() override;

	virtual int Order() const override;
};
