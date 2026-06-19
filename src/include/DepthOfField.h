#pragma once

#include "PostProcessEffect.h"
#include "Debug.h"
#include "Shader.h"

class DepthOfField : public PostProcessEffect, public ImGuiDrawable {
public:
    serialized int blurLevel = 3;
    serialized float minDistance = 5.0f;
    serialized float maxDistance = 20.0f;
    serialized float focusDistance = 50.0f;

    serialized bool useDilution = false;
    serialized int size = 2;
    serialized float separation = 2.0f;
    serialized float minThreshold = 0.1f;
    serialized float maxThreshold = 0.5f;

    // Shouldn't be here, create a separate blur effect instead
    serialized float finalMixFactor = 1.0f;
private:
    glm::vec2 savedResolution;
    GLuint dofTexture;
    ComputeShaderProgram* downsampleShader;
    ComputeShaderProgram* upsampleShader;
    ComputeShaderProgram* finalShader;
public:
    DepthOfField();

    void Awake();

    virtual void OnPostProcess(const PostProcessParams* params);

    virtual void DrawImGui();

	virtual int Order() const override;
private:
    void UpdateTexture();
};
