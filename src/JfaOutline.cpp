#include "JfaOutline.h"

#include "Shader.h"
#include "Graphics.h"
#include "imgui.h"

JfaOutline::JfaOutline() {
    this->seedShader = std::make_unique<ComputeShaderProgram>("./res/shaders/jfa_outline/seed.comp");
    this->jfaShader = std::make_unique<ComputeShaderProgram>("./res/shaders/jfa_outline/jfa.comp");
    this->finalShader = std::make_unique<ComputeShaderProgram>("./res/shaders/jfa_outline/final.comp");

    this->savedResolution = GetScene()->GetGraphics()->GetScreenResolution();

    UpdateTexture();
}

JfaOutline::~JfaOutline() {
    if (this->pingTexture != 0) {
         glDeleteTextures(1, &this->pingTexture);
    }
    if (this->pongTexture != 0) {
        glDeleteTextures(1, &this->pongTexture);
    }
}

void JfaOutline::OnPostProcess(const PostProcessParams* params) {
    if (!this->seedShader || !params) return;

    if (this->savedResolution != GetScene()->GetGraphics()->GetScreenResolution()) {
        this->savedResolution = GetScene()->GetGraphics()->GetScreenResolution();

        UpdateTexture();
    }

    glm::uvec2 resolution = glm::ceil(this->savedResolution);

    glUseProgram(this->seedShader->GetHandle());

    glUniform1i(glGetUniformLocation(this->seedShader->GetHandle(), "ignoreDepth"), this->ignoreDepth);

    glBindTextureUnit(0, params->depthTexture->GetHandle());
    glBindTextureUnit(1, params->maskTexture->GetHandle());
    glBindTextureUnit(2, params->maskDepthTexture->GetHandle());

    glBindImageTexture(0, this->pingTexture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    const auto numGroupsX = static_cast<GLuint>(std::ceil(resolution.x / 8.0f));
    const auto numGroupsY = static_cast<GLuint>(std::ceil(resolution.y / 8.0f));
    glDispatchCompute(numGroupsX, numGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);


    glUseProgram(this->jfaShader->GetHandle());

    int maxThickness = static_cast<int>(std::ceil(this->outlineThickness));
    int jumpDistance = 1;
    while (jumpDistance <= maxThickness) {
        jumpDistance *= 2;
    }
    int maxPossibleJump = std::max(resolution.x, resolution.y) / 2;
    jumpDistance = std::min(jumpDistance, maxPossibleJump);

    int i = 0;
    while (jumpDistance >= 1) {
        if (i % 2 == 0) {
            glBindImageTexture(0, this->pingTexture, 0, false, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, this->pongTexture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
        } else {
            glBindImageTexture(0, this->pongTexture, 0, false, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, this->pingTexture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
        }

        glUniform1i(glGetUniformLocation(this->jfaShader->GetHandle(), "jumpDistance"), jumpDistance);

        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        jumpDistance /= 2;
        i++;
    }

    glUseProgram(this->finalShader->GetHandle());

    glUniform1f(glGetUniformLocation(this->finalShader->GetHandle(), "outlineThickness"), this->outlineThickness);
    glUniform3fv(glGetUniformLocation(this->finalShader->GetHandle(), "outlineColor"), 1, &this->outlineColor.x);
    glUniform1i(glGetUniformLocation(this->finalShader->GetHandle(), "drawInnerLines"), this->drawInnerLines);

    glBindTextureUnit(0, params->inputTexture->GetHandle());
    if (i % 2 == 0) {
        glBindImageTexture(0, this->pingTexture, 0, false, 0, GL_READ_ONLY, GL_RGBA16F);
    } else {
        glBindImageTexture(0, this->pongTexture, 0, false, 0, GL_READ_ONLY, GL_RGBA16F);
    }
    glBindImageTexture(1, params->outputTexture->GetHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glDispatchCompute(numGroupsX, numGroupsY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void JfaOutline::DrawImGui() {
    ImGui::Text("JFA Outline");
    ImGui::Spacing();

    ImGui::SliderFloat("Thickness", &this->outlineThickness, 0.0f, 100.0f);
    ImGui::ColorEdit3("Color", &this->outlineColor.x);

    ImGui::Checkbox("Ignore Depth", &this->ignoreDepth);

    ImGui::BeginDisabled(this->ignoreDepth);
    ImGui::Checkbox("Draw Inner Lines", &this->drawInnerLines);
    ImGui::EndDisabled();
}

void JfaOutline::UpdateTexture() {
    glm::uvec2 resolution = glm::ceil(this->savedResolution);

    if (resolution.x <= 0 || resolution.y <= 0) {
        return;
    }

    if (this->pingTexture != 0) {
        glDeleteTextures(1, &this->pingTexture);
    }
    if (this->pongTexture != 0) {
        glDeleteTextures(1, &this->pongTexture);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &this->pingTexture);
    glTextureStorage2D(this->pingTexture, 1, GL_RGBA16F, resolution.x, resolution.y);
    glTextureParameteri(this->pingTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(this->pingTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(this->pingTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(this->pingTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    glCreateTextures(GL_TEXTURE_2D, 1, &this->pongTexture);
    glTextureStorage2D(this->pongTexture, 1, GL_RGBA16F, resolution.x, resolution.y);
    glTextureParameteri(this->pongTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(this->pongTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(this->pongTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(this->pongTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
}
