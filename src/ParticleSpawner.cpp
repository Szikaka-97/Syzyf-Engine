#include "ParticleSpawner.h"

#include "Layer.h"
#include "Material.h"
#include "TimeSystem.h"
#include "Graphics.h"

#include <glm/gtc/random.hpp>

ParticleSpawner::ParticleSpawner(Mesh* mesh, Material* material, glm::vec3 areaExtents, int maxParticles) : mesh(mesh), material(material), areaExtents(areaExtents), maxParticles(maxParticles) {
    ComputeShader* shader = this->GetScene()->Resources()->Get<ComputeShader>("res/shaders/particles/particles.comp");
    this->computeShader.reset(new ComputeShaderProgram(shader));

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 min = -areaExtents;
    glm::vec3 max = areaExtents;

    this->initialParticleData.reserve(this->maxParticles);

    for (int i = 0; i < this->maxParticles; i++) {
        ParticleData p;

        glm::vec3 randomPosition = center + glm::linearRand(min * 0.5f, max * 0.5f);
        glm::vec3 randomVelocity = {
            0.0f,
            glm::linearRand(-0.5f, -0.1f),
            0.0f,
        };

        p.position = glm::vec4(randomPosition, 1.0f);
        p.velocity = glm::vec4(randomVelocity, 0.0f);

        this->initialParticleData.push_back(p);
    }

    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * sizeof(ParticleData), this->initialParticleData.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

ParticleSpawner::~ParticleSpawner() {
    if (this->particleBuffer != 0) {
        glDeleteBuffers(1, &this->particleBuffer);
    }
}

void ParticleSpawner::Update() {
    glUseProgram(this->computeShader->GetHandle());

    glm::vec3 center = this->GlobalTransform().Position();
    glm::vec3 extents = areaExtents;

    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaCenter"), 1, &center[0]);
    glUniform3fv(glGetUniformLocation(this->computeShader->GetHandle(), "uAreaExtents"), 1, &extents[0]);
    glUniform1f(glGetUniformLocation(this->computeShader->GetHandle(), "uDeltaTime"), Time::Delta());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->particleBuffer);
    glDispatchCompute(maxParticles / 64, 1, 1);
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
        this->maxParticles,
        BoundingBox::CenterAndExtents(glm::vec3(0.0f), this->areaExtents),
        Layer::Default
        );
}
