#include "ParticleSpawner.h"

#include "Layer.h"
#include "Material.h"
#include "TimeSystem.h"
#include "Graphics.h"

#include <glm/gtc/random.hpp>

ParticleSpawner::ParticleSpawner(Mesh* mesh, Material* material, ParticleSpawnerSettings settings) : mesh(mesh), material(material), settings(settings) {
    ComputeShader* shader = this->GetScene()->Resources()->Get<ComputeShader>("res/shaders/particles/particles.comp");
    this->computeShader.reset(new ComputeShaderProgram(shader));

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 min = -settings.areaExtents;
    glm::vec3 max = settings.areaExtents;

    this->initialParticleData.reserve(settings.maxParticles);

    for (int i = 0; i < settings.maxParticles; i++) {
        ParticleData p;

        glm::vec3 randomPosition = center + glm::linearRand(min, max);
        glm::vec3 randomVelocity = glm::linearRand(settings.minVelocity, settings.maxVelocity);
        float randomScale = glm::linearRand(settings.minScale, settings.maxScale);

        p.position = glm::vec4(randomPosition, randomScale);
        p.velocity = glm::vec4(randomVelocity, 1.0f);

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
    // change later
    this->material->SetValue("billboardMode", static_cast<unsigned int>(this->settings.billboardMode));

    glUseProgram(this->computeShader->GetHandle());

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 extents = this->settings.areaExtents;

    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaCenter"), 1, &center[0]);
    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaExtents"), 1, &extents[0]);
    glUniform1f(glGetUniformLocation(this->computeShader->GetHandle(), "uDeltaTime"), Time::Delta());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->particleBuffer);
    glDispatchCompute(this->settings.maxParticles / 64, 1, 1);
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
