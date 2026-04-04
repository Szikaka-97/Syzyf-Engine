#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

struct ParticleData {
    glm::vec4 position;
    glm::vec4 velocity;
};

enum class BillboardMode {
    Disabled = 0,
    Enabled = 1,
    Z = 2,
};

struct ParticleSpawnerSettings {
    int maxParticles = 1024;
    glm::vec3 areaExtents = glm::vec3(50.0f);

    glm::vec3 minVelocity = { 0.0f, -1.0f, 0.0f };
    glm::vec3 maxVelocity = { 0.0f, -0.2f, 0.0f };

    float minScale = 1.0f;
    float maxScale = 1.0f;

    BillboardMode billboardMode = BillboardMode::Disabled; 
};

class ParticleSpawner : public GameObject {
private:
    std::unique_ptr<ComputeShaderProgram> computeShader;

    Mesh* mesh = nullptr;
    Material* material = nullptr;
    ParticleSpawnerSettings settings;

    std::vector<ParticleData> initialParticleData;
    GLuint particleBuffer;
public:
    ParticleSpawner(Mesh* mesh, Material* material, ParticleSpawnerSettings = {});
    ~ParticleSpawner();

    void Update();
    void Render();
};
