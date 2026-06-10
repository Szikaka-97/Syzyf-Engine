#pragma once

#include "Debug.h"
#include "GameObject.h"
#include "GameObjectSystem.h"
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

enum class AlphaMode {
    Disabled = 0,
    Alpha = 1,
    Dither = 2,
};

// somethings broken, turning off lifetimes doesnt work i dont think
struct ParticleSpawnerSettings {
    serialized int maxParticles = 1024;

    // Area which if exceeded teleports the particle to the opposite end
    //  maybe change it so you can control this using node's scale instead
    serialized glm::vec3 areaExtents = glm::vec3(50.0f);

    // Particles will spawn at a random point in this area
    //  for now only a box shape
    serialized glm::vec3 emissionShapeExtents = glm::vec3(0.1f);

    serialized glm::vec3 minVelocity = { 0.0f, -1.0f, 0.0f };
    serialized glm::vec3 maxVelocity = { 0.0f, -0.2f, 0.0f };

    // in radians
    serialized float minInitialAngle = 0.0f;
    serialized float maxInitialAngle = 0.0f;

    serialized float minAngularVelocity = 0.0f;
    serialized float maxAngularVelocity = 0.0f;

    serialized bool rotateY = false;

    serialized bool enableLifetime = false;
    // The time until the particle 'despawns'
    serialized float minLifetime = 2.0f;
    serialized float maxLifetime = 2.0f;

    serialized float minScale = 1.5f;
    serialized float maxScale = 1.5f;

    // Controls how the scale changes over the particle's lifetime
    Texture2D* scaleCurveTexture = nullptr;

    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float colorIntensity = 1.0f;

    // This is a bit silly because since you set the frag shader manually this setting doesn't really change much
    //  besides hiding the imgui options
    serialized AlphaMode alphaMode = AlphaMode::Disabled;
    // Changes whether the particles should fade out as they get closer to the camera
    serialized bool enableProximityFade = false;
    serialized float proximityFadeMin = 0.0f;
    serialized float proximityFadeMax = 10.0f;

    // Changes whether the particles should fade as they get closer to the particle spawner area extents
    serialized bool enableDistanceFade = false;
    serialized float distanceFadeMin = 30.0f;
    serialized float distanceFadeMax = 40.0f;

    serialized bool enableLifetimeFade = false;
    serialized glm::vec2 lifetimeFadeIn = { 0.0f, 0.2f };
    serialized glm::vec2 lifetimeFadeOut = { 0.8f, 1.0f };

    // Fades out the intersections between the particle and scene geometry
    serialized bool enableDepthFade = false;
    serialized float depthFadeDistance = 1.5f;

    serialized BillboardMode billboardMode = BillboardMode::Disabled;

    // Teleports particles to the opposite end of the area if they go outside it
    serialized bool wrapAround = false;

    serialized bool continuous = false;
    
    // maybe have the particlespawner hold textures itself instead?
    serialized bool useColorRamp = false;
};

class ParticleSpawner : public GameObject, public ImGuiDrawable {
private:
    const std::filesystem::path COMPUTE_SHADER_PATH = "res/shaders/particles/particles.comp";
    const std::filesystem::path DITHER_TEXTURE_PATH = "./res/textures/bayer/bayer16.png";

    serialized Mesh* mesh = nullptr;
    serialized Texture2D* ditherTexture = nullptr;
    serialized Material* material = nullptr;
    std::unique_ptr<ComputeShaderDispatch> computeDispatch;

    serialized ParticleSpawnerSettings settings;

    std::vector<ParticleData> initialParticleData;
    GLuint particleBuffer = 0;
    bool particleBufferBoundToMaterial = false;

    void ReallocateParticleBuffer();
public:
    ParticleSpawner() = default;
    ParticleSpawner(Mesh* mesh, Material* material, ParticleSpawnerSettings = {});
    ~ParticleSpawner();

    void Awake();

    void Update();
    void Render();

    void DrawImGui();
};

class ParticleSpawnerSystem : public GameObjectSystem<ParticleSpawner> {
public:
    ParticleSpawnerSystem(Scene* scene) : GameObjectSystem<ParticleSpawner>(scene) {}
};
