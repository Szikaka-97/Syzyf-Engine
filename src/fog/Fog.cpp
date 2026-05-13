#include "fog/Fog.h"
#include "LightSystem.h"
#include "Graphics.h"
#include "Shader.h"
#include "imgui.h"

Fog::Fog(Fog::Type fogType, float minDistance, float maxDistance, glm::vec4 fogColor) : minDistance(minDistance), maxDistance(maxDistance), fogColor(fogColor) {
  this->shader = new ComputeShaderProgram("./res/shaders/fog/fog.comp"); 
}

void Fog::OnPostProcess(const PostProcessParams* params) {
    glUseProgram(this->shader->GetHandle());

    glUniform1ui(glGetUniformLocation(this->shader->GetHandle(), "fogType"), static_cast<unsigned int>(this->fogType));
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "minDistance"), this->minDistance);
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "maxDistance"), this->maxDistance);
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "density"), this->density);
    glUniform4fv(glGetUniformLocation(this->shader->GetHandle(), "fogColor"), 1, &this->fogColor[0]);

  if (this->fogType == Type::Atmospheric) {
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "fogHeightFalloff"), this->fogHeightFalloff);
    glUniform1f(glGetUniformLocation(this->shader->GetHandle(), "inscatteringPower"), this->inscatteringPower);

    // Fallback
    glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.2f));
    glm::vec3 sunColor = glm::vec3(1.0f, 0.9f, 0.7f);

    LightSystem* lightSystem = GetScene()->GetComponent<LightSystem>();
    if (lightSystem) {
        for (Light* light : lightSystem->IterateObjects()) {
            if (light->GetType() == Light::LightType::Directional) {
                sunDirection = glm::normalize(-light->GlobalTransform().Forward());
                sunColor = light->GetColor() * light->GetIntensity();
                break;
            }
        }
    }

        glUniform3fv(glGetUniformLocation(this->shader->GetHandle(), "sunDirection"), 1, &sunDirection[0]);
        glUniform3fv(glGetUniformLocation(this->shader->GetHandle(), "sunColor"), 1, &sunColor[0]);
    }

    glBindTextureUnit(0, params->inputTexture->GetHandle());
    glBindTextureUnit(1, params->depthTexture->GetHandle());

    glBindImageTexture(0, params->outputTexture->GetHandle(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
    glDispatchCompute(std::ceil(resolution.x / 8.0f), std::ceil(resolution.y / 8.0f), 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Fog::DrawImGui() {
    const char* fogTypes[] = { "Linear", "Exponential", "Exponential Squared", "Atmospheric" };
    int currentType = (int) this->fogType;
    if (ImGui::Combo("FogType", &currentType, fogTypes, IM_ARRAYSIZE(fogTypes))) {
        this->fogType = (Type) currentType;
    }

    ImGui::ColorPicker4("Fog Color", &this->fogColor.x);

    if (this->fogType == Type::Linear) {
        ImGui::InputFloat("Min Distance", &this->minDistance);
        ImGui::InputFloat("Max Distance", &this->maxDistance);
    } else if (this->fogType == Type::Atmospheric) {
        ImGui::DragFloat("Density", &this->density, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Height Falloff", &this->fogHeightFalloff, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Inscattering Power", &this->inscatteringPower, 0.1f, 1.0f, 128.0f);
    } else {
        ImGui::DragFloat("Density", &this->density, 0.001f, 0.0f, 1.0f);
    }
}
