#pragma once

#include <PostProcessEffect.h>
#include <Material.h>           // ComputeShaderProgram
#include <glm/glm.hpp>
#include "Debug.h"

class MotionBlur : public PostProcessEffect, public ImGuiDrawable {
public:
    MotionBlur();
    ~MotionBlur();

    void OnPostProcess(const PostProcessParams* params) override;
    virtual void DrawImGui() override;           // opcjonalne sterowanie parametrami

    void SetEnabled(bool enabled);
    void SetSamples(int samples);
    void SetSeparation(float separation);
     bool      enabled = true;
private:
    ComputeShaderProgram* motionBlurShader;

    glm::mat4 previousViewProjection;  // poprzednia klatka: ViewProj
    bool      firstFrame = true;

   
    int       samples = 32;            // liczba próbek
    float     separation = 0.01f;       // skala separacji wektora ruchu

    void UpdateUniforms();
};
