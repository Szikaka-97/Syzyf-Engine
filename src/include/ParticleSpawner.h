#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

struct ParticleData {
    // xyz - position, w - size
    glm::vec4 position;
    // xyz - velocity, w - alpha
    glm::vec4 velocity;
    // x - current age, y - max age, z - initial angle, w - angular velocity
    glm::vec4 lifetime;
};

enum class BillboardMode {
    Disabled = 0,
    Enabled = 1,
    Z = 2,
};

enum class FadeMode {
    Disabled = 0,
    Alpha = 1,
    Dither = 2,
};

struct ParticleSpawnerSettings {
    int maxParticles = 1024;

    // Area which if exceeded teleports the particle to the opposite end
    //  maybe change it so you can control this using node's scale instead
    glm::vec3 areaExtents = glm::vec3(50.0f);

    // Particles will spawn at a random point in this area
    //  for now only a box shape
    glm::vec3 emissionShapeExtents = glm::vec3(0.1f);

    glm::vec3 minVelocity = { 0.0f, -1.0f, 0.0f };
    glm::vec3 maxVelocity = { 0.0f, -0.2f, 0.0f };

    float minInitialAngle = 0.0f;
    float maxInitialAngle = 0.0f;
    float minAngularVelocity = 0.0f;
    float maxAngularVelocity = 0.0f;

    bool enableLifetime = false;
    // The time until the particle 'despawns'
    float minLifetime = 2.0f;
    float maxLifetime = 2.0f;

    float minScale = 1.5f;
    float maxScale = 1.5f;

    // Changes whether the particles should fade out as they get closer to the camera
    FadeMode proximityFadeMode = FadeMode::Disabled;
    float proximityFadeMin = 0.0f;
    float proximityFadeMax = 10.0f;

    // Changes whether the particles should fade as they get closer to the particle spawner area extents
    FadeMode distanceFadeMode = FadeMode::Disabled;
    float distanceFadeMin = 30.0f;
    float distanceFadeMax = 40.0f;

    BillboardMode billboardMode = BillboardMode::Disabled;

    bool wrapAround = false;
    bool continuous = false;
};

class ParticleSpawner : public GameObject, public ImGuiDrawable {
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

    void DrawImGui();
};
