#include "MotionBlur.h"
#include <Graphics.h>
#include <Scene.h>
#include <imgui.h>
#include <filesystem>

MotionBlur::MotionBlur() {
    if( std::filesystem::exists("./res/shaders/motion_blur/motion_blur.comp"))
        spdlog::error("istnieje");
    else spdlog::error("nie istnieje");
    motionBlurShader = new ComputeShaderProgram(
        "./res/shaders/motion_blur/motion_blur.comp");
    previousViewProjection = glm::mat4(1.0f);
}

MotionBlur::~MotionBlur() {
    delete motionBlurShader;
}

void MotionBlur::SetEnabled(bool enabled) {
    this->enabled = enabled;
}

void MotionBlur::SetSamples(int samples) {
    this->samples = glm::max(1, samples);
}

void MotionBlur::SetSeparation(float separation) {
    this->separation = glm::max(0.0f, separation);
}

void MotionBlur::UpdateUniforms() {
    GLuint program = motionBlurShader->GetHandle();
    glUseProgram(program);

    glUniform2f(glGetUniformLocation(program, "u_motionBlurEnabled"),
                enabled ? 1.0f : 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(program, "u_parameters"),
                static_cast<float>(samples), separation);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_previousViewProjection"),
                       1, GL_FALSE, &previousViewProjection[0][0]);
}

void MotionBlur::OnPostProcess(const PostProcessParams* params) {
    if (!motionBlurShader || !params)
        return;

    glUseProgram(motionBlurShader->GetHandle());

    glBindTextureUnit(0, params->inputTexture->GetHandle());
    glBindTextureUnit(1, params->depthTexture->GetHandle());
    glBindImageTexture(0, params->outputTexture->GetHandle(),
                       0, false, 0, GL_READ_WRITE, GL_RGBA16F);

    // Pobranie macierzy z g��wnej kamery
    Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
    glm::mat4 proj = camera ? camera->ProjectionMatrix() : glm::mat4(1.0f);
    glm::mat4 view = camera ? camera->ViewMatrix() : glm::mat4(1.0f);
    glm::mat4 invProj = glm::inverse(proj);
    glm::mat4 invView = glm::inverse(view);

    GLuint program = motionBlurShader->GetHandle();
    glUniformMatrix4fv(glGetUniformLocation(program, "u_invProjection"), 1, GL_FALSE, &invProj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_invView"), 1, GL_FALSE, &invView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_previousViewProjection"), 1, GL_FALSE, &previousViewProjection[0][0]);

    glUniform2f(glGetUniformLocation(program, "u_motionBlurEnabled"),
                enabled ? 1.0f : 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(program, "u_parameters"),
                static_cast<float>(samples), separation);

    glm::uvec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
    glDispatchCompute((resolution.x + 7) / 8, (resolution.y + 7) / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Zachowanie macierzy ViewProj na nast�pn� klatk�
    previousViewProjection = proj * view;
}

void MotionBlur::DrawImGui() {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::InputInt("Samples", &samples);
    ImGui::InputFloat("Separation", &separation);
    samples = glm::max(1, samples);
    separation = glm::max(0.0f, separation);
}

int MotionBlur::Order() const {
    return 6;
}