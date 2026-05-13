#pragma once

#include "PostProcessEffect.h"
#include "Debug.h"

#include <glad/glad.h>
#include <glm/common.hpp>

class ComputeShaderProgram;

class JfaOutline : public PostProcessEffect, public ImGuiDrawable {
public:
    bool ignoreDepth = false;
    bool drawInnerLines = false;

    glm::vec3 outlineColor = glm::vec3(1.0f);
    float outlineThickness = 10.0f;
private:
    glm::vec2 savedResolution;

    GLuint pingTexture = 0;
    GLuint pongTexture = 0;

    std::unique_ptr<ComputeShaderProgram> seedShader;
    std::unique_ptr<ComputeShaderProgram> jfaShader;
    std::unique_ptr<ComputeShaderProgram> finalShader;
public:
    JfaOutline();
    ~JfaOutline();

    virtual void OnPostProcess(const PostProcessParams* params);

    virtual void DrawImGui();
private:
    void UpdateTexture();
};
