#include "ParticleSpawner.h"

#include "Layer.h"
#include "Material.h"
#include "TimeSystem.h"
#include "Graphics.h"

#include "imgui.h"
#include <glm/gtc/random.hpp>

ParticleSpawner::ParticleSpawner(Mesh* mesh, Material* material, ParticleSpawnerSettings settings) : mesh(mesh), material(material), settings(settings) {
    ComputeShader* shader = this->GetScene()->Resources()->Get<ComputeShader>("res/shaders/particles/particles.comp");
    this->computeShader.reset(new ComputeShaderProgram(shader));

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 min = -settings.emissionShapeExtents;
    glm::vec3 max = settings.emissionShapeExtents;

    this->initialParticleData.reserve(settings.maxParticles);

    for (int i = 0; i < settings.maxParticles; i++) {
        ParticleData p;

        glm::vec3 randomPosition = center + glm::linearRand(min, max);
        glm::vec3 randomVelocity = glm::linearRand(settings.minVelocity, settings.maxVelocity);

        float lifetimeFraction = static_cast<float>(i) / static_cast<float>(settings.maxParticles);
        float initialLifetime = lifetimeFraction * settings.maxLifetime;
        float randomLifetime = glm::linearRand(settings.minLifetime, settings.maxLifetime);

        float randomAngle = glm::linearRand(settings.minInitialAngle, settings.maxInitialAngle);
        float randomAngularVelocity = glm::linearRand(settings.minAngularVelocity, settings.maxAngularVelocity);

        float randomScale = glm::linearRand(settings.minScale, settings.maxScale);

        p.position = glm::vec4(randomPosition, randomScale);
        p.velocity = glm::vec4(randomVelocity, 1.0f);
        p.lifetime.x = initialLifetime;
        p.lifetime.y = randomLifetime;
        p.lifetime.z = randomAngle;
        p.lifetime.w = randomAngularVelocity;

        this->initialParticleData.push_back(p);
    }

    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, settings.maxParticles * sizeof(ParticleData), this->initialParticleData.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

ParticleSpawner::~ParticleSpawner() {
    if (this->particleBuffer != 0) {
        glDeleteBuffers(1, &this->particleBuffer);
    }
}

void ParticleSpawner::Update() {
    // rename so either nothing has the 'u' prefix or every uniform has it
    this->material->SetValue("areaCenter", this->GlobalTransform().Position().value);

    // change later
    this->material->SetValue("billboardMode", static_cast<unsigned int>(this->settings.billboardMode));

    this->material->SetValue("proximityFadeMode", static_cast<unsigned int>(this->settings.proximityFadeMode));
    if (this->settings.proximityFadeMode != FadeMode::Disabled) {
        this->material->SetValue("proximityFadeMin", this->settings.proximityFadeMin);
        this->material->SetValue("proximityFadeMax", this->settings.proximityFadeMax);
    }

    this->material->SetValue("distanceFadeMode", static_cast<unsigned int>(this->settings.distanceFadeMode));
    if (this->settings.distanceFadeMode != FadeMode::Disabled) {
        this->material->SetValue("distanceFadeMin", this->settings.distanceFadeMin);
        this->material->SetValue("distanceFadeMax", this->settings.distanceFadeMax);
    }

    glUseProgram(this->computeShader->GetHandle());

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 extents = this->settings.areaExtents;

    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaCenter"), 1, &center[0]);
    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uEmissionShapeExtents"), 1, &this->settings.emissionShapeExtents[0]);
    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaExtents"), 1, &extents[0]);
    glUniform1f(glGetUniformLocation(this->computeShader->GetHandle(), "uDeltaTime"), Time::Delta());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->particleBuffer);
    GLuint workGroups = (this->settings.maxParticles + 63) / 64;
    glDispatchCompute(workGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    // will break if another ssbo gets bound to 3 ,fix
}

void ParticleSpawner::Render() {
    if (!this->mesh || !this->material) {
        spdlog::error("ParticleSpawner: mesh or material missing");
        return;
    }

    this->GetScene()->GetGraphics()->DrawMeshInstanced(
        this->mesh,
        0,
        this->material,
        this->GlobalTransform(),
        this->settings.maxParticles,
        BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->settings.areaExtents),
        Layer::Default
    );
}

void ParticleSpawner::DrawImGui() {
    const char* billboardModes[] = { "Disabled", "Enabled", "Z" };
    int currentBillboardMode = static_cast<int>(this->settings.billboardMode);
    if (ImGui::Combo("Billboard Mode", &currentBillboardMode, billboardModes, IM_ARRAYSIZE(billboardModes))) {
        this->settings.billboardMode = static_cast<BillboardMode>(currentBillboardMode);
    }

    const char* fadeModes[] = { "Disabled", "Alpha", "Dither" };

    int currentProximityFadeMode = static_cast<int>(this->settings.proximityFadeMode);
    if (ImGui::Combo("Proximity Fade Mode", &currentProximityFadeMode, fadeModes, IM_ARRAYSIZE(fadeModes))) {
        this->settings.proximityFadeMode= static_cast<FadeMode>(currentProximityFadeMode);
    }
    if (this->settings.proximityFadeMode != FadeMode::Disabled) {
        ImGui::InputFloat("Proximity Fade Min", &this->settings.proximityFadeMin);
        ImGui::InputFloat("Proximity Fade Max", &this->settings.proximityFadeMax);
    }

    int currentDistanceFadeMode = static_cast<int>(this->settings.distanceFadeMode);
    if (ImGui::Combo("Distance Fade Mode", &currentDistanceFadeMode, fadeModes, IM_ARRAYSIZE(fadeModes))) {
        this->settings.distanceFadeMode = static_cast<FadeMode>(currentDistanceFadeMode);
    }
    if (this->settings.distanceFadeMode != FadeMode::Disabled) {
        ImGui::InputFloat("Distance Fade Min", &this->settings.distanceFadeMin);
        ImGui::InputFloat("Distance Fade Max", &this->settings.distanceFadeMax);
    }
}
