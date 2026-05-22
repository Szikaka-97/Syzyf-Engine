#pragma once

#include "PostProcessEffect.h"
#include "Debug.h"
#include "Shader.h"

class DepthOfField : public PostProcessEffect, public ImGuiDrawable {
private:
    glm::vec2 savedResolution;
    GLuint dofTexture;
    ComputeShaderProgram* downsampleShader;
    ComputeShaderProgram* upsampleShader;
    ComputeShaderProgram* finalShader;

    int blurLevel = 3;
    float minDistance = 5.0f;
    float maxDistance = 20.0f;
    float focusDistance = 50.0f;

    bool useDilution = false;
    int size = 2;
    float separation = 2.0f;
    float minThreshold = 0.1f;
    float maxThreshold = 0.5f;
public:
    DepthOfField();

    virtual void OnPostProcess(const PostProcessParams* params);

    virtual void DrawImGui();
private:
    void UpdateTexture();
};
