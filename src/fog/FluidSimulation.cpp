#include "fog/FluidSimulation.h"

#include "Texture.h"
#include "TimeSystem.h"
#include "game_scripts/PlayerController.h"
#include "imgui.h"

FluidSimulation::FluidSimulation() {
    TextureParams params;
    params.channels = TextureChannels::RG;
    params.format = TextureFormat::Float;
    params.colorSpace = TextureColor::Linear;
    params.wrapU = TextureWrap::Clamp;
    params.wrapV = TextureWrap::Clamp;
    params.minFilter = TextureFilter::Linear;
    params.magFilter = TextureFilter::Linear;

    textureRead = new Texture2D(resolution, resolution, params);
    textureWrite = new Texture2D(resolution, resolution, params);

    computeProgram = new ComputeShaderProgram("./res/shaders/fog/velocity_sim.comp");
}

void FluidSimulation::Update() {
    auto players = GetScene()->FindObjectsOfType<PlayerController>();
    if (players.empty() || players[0] == nullptr) return;
        
    glm::vec3 playerPosition = players[0]->GetNode()->GlobalTransform().Position().Value();

    if (glm::length(lastPlayerPosition) < 0.001f) {
        lastPlayerPosition = playerPosition;
    }

    glm::vec2 playerVelocity = glm::vec2(playerPosition.x - lastPlayerPosition.x, playerPosition.z - lastPlayerPosition.z);

    if (Time::Delta() > 0.0f) {
        playerVelocity = (playerVelocity / Time::Delta()) * interactionStrength;
    }

    glm::vec3 simCenter = GlobalTransform().Position().Value();
    glm::vec3 simSize = GlobalTransform().Scale().Value();
    glm::vec2 simSize2D = glm::vec2(simSize.x, simSize.z);

    glm::vec2 playerUV;
    playerUV.x = (playerPosition.x - (simCenter.x - simSize.x * 0.5f)) / simSize.x;
    playerUV.y = (playerPosition.z - (simCenter.z - simSize.z * 0.5f)) / simSize.z;

    GLuint handle = computeProgram->GetHandle();
    glUseProgram(handle);

    glUniform2fv(glGetUniformLocation(handle, "playerPosUV"), 1, &playerUV[0]);
    glUniform2fv(glGetUniformLocation(handle, "playerVelocity"), 1, &playerVelocity[0]);
    glUniform1f(glGetUniformLocation(handle, "playerRadius"), this->playerRadius);
    glUniform1f(glGetUniformLocation(handle, "dt"), Time::Delta());
    glUniform1f(glGetUniformLocation(handle, "damping"), this->damping);
    glUniform2fv(glGetUniformLocation(handle, "simSize"), 1, &simSize2D[0]);

    glBindTextureUnit(0, textureRead->GetHandle());
    glUniform1i(glGetUniformLocation(handle, "texVelocityRead"), 0);

    glBindImageTexture(1, textureWrite->GetHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);

    glDispatchCompute(std::ceil(resolution / 16.0f), std::ceil(resolution / 16.0f), 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::swap(textureRead, textureWrite);

    lastPlayerPosition = playerPosition;
}

Texture2D* FluidSimulation::GetVelocityMap() {
    return textureRead;
}

void FluidSimulation::DrawImGui() {
    ImGui::Text("Fluid Simulation Settings");
    ImGui::SliderFloat("Damping", &this->damping, 0.9f, 1.0f);
    ImGui::SliderFloat("Player Radius", &this->playerRadius, 0.01f, 0.2f);
    ImGui::SliderFloat("Interaction Strength", &this->interactionStrength, 0.0f, 5.0f);

    if (this->textureRead != nullptr) {
        ImGui::Spacing();
        ImGui::Text("Velocity Map:");
        
        ImGui::Image((void*)(intptr_t)this->textureRead->GetHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    }
}
