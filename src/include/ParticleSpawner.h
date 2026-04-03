#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

struct ParticleData {
    glm::vec4 position;
    glm::vec4 velocity;
};

class ParticleSpawner : public GameObject {
private:
    std::unique_ptr<ComputeShaderProgram> computeShader;
    Mesh* mesh = nullptr;
    Material* material = nullptr;

    int maxParticles;
    std::vector<ParticleData> initialParticleData;
    glm::vec3 areaExtents;

    GLuint particleBuffer;
public:
    ParticleSpawner(Mesh* mesh, Material* material, glm::vec3 extents = glm::vec3(50.0f), int maxParticles = 1024);
    ~ParticleSpawner();

    void Update();
    void Render();
};
