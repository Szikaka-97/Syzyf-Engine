#include "MaskEffects.h"

#include "PostProcessEffect.h"
#include "Shader.h"
#include "Graphics.h"

#include <imgui.h>

MaskEffects::MaskEffects() {
    this->shader = new ComputeShaderProgram("./res/shaders/mask/mask.comp");
}

void MaskEffects::OnPostProcess(const PostProcessParams* params) {
    if (!this->shader || !params) return;

    const glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();

    glUseProgram(this->shader->GetHandle());

    glUniform4fv(glGetUniformLocation(this->shader->GetHandle(), "xrayColor"), 1, &this->xrayColor[0]);
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "xrayIntensity"), this->xrayIntensity);
    glUniform4fv(glGetUniformLocation(this->shader->GetHandle(), "outlineColor"), 1, &this->outlineColor[0]);
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "outlineIntensity"), this->outlineIntensity);

    glBindTextureUnit(0, params->inputTexture->GetHandle());

    glBindTextureUnit(1, params->depthTexture->GetHandle());
    glBindTextureUnit(2, params->maskTexture->GetHandle());
    glBindTextureUnit(3, params->maskDepthTexture->GetHandle());

    glBindImageTexture(0, params->outputTexture->GetHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    const auto numGroupsX = static_cast<GLuint>(std::ceil(resolution.x / 8.0f));
    const auto numGroupsY = static_cast<GLuint>(std::ceil(resolution.y / 8.0f));
    glDispatchCompute(numGroupsX, numGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void MaskEffects::DrawImGui() {
    ImGui::Text("Mask Effects");
    ImGui::Spacing();
    ImGui::ColorEdit4("X-Ray Color", &this->xrayColor[0]);
    ImGui::InputFloat("X-Ray Intensity", &this->xrayIntensity);
    ImGui::Separator();
    ImGui::ColorEdit4("Outline Color", &this->outlineColor[0]);
    ImGui::InputFloat("Outline Intensity", &this->outlineIntensity);
}

int MaskEffects::Order() const {
    return 2;
}