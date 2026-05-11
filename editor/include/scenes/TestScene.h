#pragma once

#include "DepthOfField.h"
#include "EasingFunctions.h"
#include "GltfImporter.h"
#include "LightSystem.h"
#include "fog/Fog.h"
#include "game_scripts/AimingAid.h"
#include "game_scripts/ThrowBottle.h"

#include "Noise3D.h"
#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <MeshRenderer.h>
#include <Mirror.h>
#include <ParticleSpawner.h>
#include <Player.h>
#include <ReflectionProbe.h>
#include <Scene.h>
#include <Shader.h>
#include <Skybox.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <TweenSystem.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <enemies/EnemySkeleton.cpp>
#include <fog/FogVolume.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/ThrowBottle.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/Helpers.h>
#include <physics/System.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>
#include <ui/UiLayout.h>
#include <ui/UiLayoutSystem.h>
#include <ui/UiVisual.h>

#include "Jolt/Math/Vec3.h"
#include "ui/UiRenderSystem.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace TestScene {
class Mover : public GameObject, public ImGuiDrawable {
  private:
    float pitch;
    float rotation;
    bool movementEnabled = false;
    int mode;
    float movementSpeed = 10.0f;
    float mouseSensitivity = 1.0f;

  public:
    Mover() {
        this->pitch = 0;
        this->rotation = 0;
        this->mode = 0;
    }

    void Update() {
        if (movementEnabled) {
            glm::vec3 movement = glm::zero<glm::vec3>();
            glm::quat rotation = glm::identity<glm::quat>();
            float movementSpeed = this->movementSpeed;

            glm::vec3 right = this->GlobalTransform().Right();
            glm::vec3 up = glm::vec3(0, 1, 0);
            glm::vec3 forward = mode == 0 ? glm::cross(right, up)
                                          : this->GlobalTransform().Forward();

            if (GetScene()->Input()->KeyPressed(Key::A)) {
                movement += right;
            }
            if (GetScene()->Input()->KeyPressed(Key::D)) {
                movement -= right;
            }
            if (GetScene()->Input()->KeyPressed(Key::W)) {
                movement += forward;
            }
            if (GetScene()->Input()->KeyPressed(Key::S)) {
                movement -= forward;
            }
            if (GetScene()->Input()->KeyPressed(Key::E)) {
                movement += up;
            }
            if (GetScene()->Input()->KeyPressed(Key::Q)) {
                movement -= up;
            }
            if (GetScene()->Input()->KeyPressed(Key::LeftShift)) {
                movementSpeed *= 2;
            }

            if (glm::length(movement) > 0.0f) {
                movement = glm::normalize(movement);
            }

            this->GlobalTransform().Position() +=
                movement * (movementSpeed * Time::Delta());

            glm::vec2 deltaMovement = GetScene()->Input()->GetMouseMovement();

            this->rotation -= (deltaMovement.x / 20) * this->mouseSensitivity;
            this->pitch -= (deltaMovement.y / 20) * this->mouseSensitivity;

            if (this->rotation < -180) {
                this->rotation += 360;
            } else if (this->rotation > 180) {
                this->rotation -= 360;
            }

            this->pitch = glm::clamp(this->pitch, -89.0f, 89.0f);

            this->GlobalTransform().Rotation() =
                glm::angleAxis(glm::radians(this->rotation),
                               glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(this->pitch), glm::vec3(1, 0, 0));

            this->GlobalTransform().Rotation() =
                this->GlobalTransform().Rotation().value;
        }

        if (GetScene()->Input()->KeyDown(Key::Escape)) {
            this->movementEnabled = !this->movementEnabled;
            GetScene()->Input()->SetMouseLocked(this->movementEnabled);

            if (this->movementEnabled) {
                glm::vec3 forward = this->GlobalTransform().Forward();
                this->pitch =
                    glm::degrees(asin(glm::clamp(-forward.y, -1.0f, 1.0f)));
                this->rotation = glm::degrees(atan2(forward.x, forward.z));
            }
        }
    }

    virtual void DrawImGui() {
        const char* modes[]{
            "Walking",
            "Freecam",
        };

        ImGui::Combo("Movement type", &this->mode, modes, 2);

        ImGui::InputFloat("Movement speed", &this->movementSpeed);
        ImGui::InputFloat("Mouse sensitivity", &this->mouseSensitivity);
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiLayoutSystem>();
    mainScene.AddComponent<UiRenderSystem>();
    mainScene.AddComponent<AnimationSystem>();
    auto* tweenSystem = mainScene.AddComponent<TweenSystem>();

// If Visual Studio doesn't like this I'm going to give up and force you guys to
// switch to GCC
#pragma region World

    ShaderProgram* skyProg =
        ShaderProgram::Build()
            .WithVertexShader(("./res/shaders/skybox.vert"))
            .WithPixelShader(("./res/shaders/skybox.frag"))
            .Link();

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/citrus_orchard_road_puresky.hdr",
        Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

    auto floorNode = GltfImporter::LoadScene(
        &mainScene, "./res/models/floor2804.glb", "Floor");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});
    floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 1.0f);
    // floorNode->GetObject<Surface>()->DrawDebugSurface();
    floorNode->GetObject<Surface>()->SetID(0);
    auto* navGrid = floorNode->AddObject<NavigationGrid>();
    navGrid->Build(floorNode->GetObject<Surface>(), 2.0f, 45.0f);

    SceneNode* monkey = GltfImporter::LoadScene(
        &mainScene, "./res/models/big_monkey.glb", "Monkey", floorNode);
    JPH::ShapeRefC monkeyShape = Physics::CreateCompoundShapeFromNode(
        monkey, false, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
    monkey->AddObject<Physics::Body>(JPH::BodyCreationSettings{
        monkeyShape, JPH::Vec3::sZero(), JPH::Quat::sIdentity(),
        JPH::EMotionType::Static, Physics::Layers::MOVING});

#pragma endregion
#pragma region Player

    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
        new JPH::CharacterVirtualSettings();
    characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
    characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    SceneNode* playerNode = mainScene.CreateNode("Player");

    SceneNode* bimberman = GltfImporter::LoadScene(
        &mainScene, "./res/models/bimbermann_throwing.glb", "Bimberman");
    bimberman->SetParent(playerNode);

    auto* virtualCharacter =
        playerNode->AddObject<Physics::VirtualCharacterController>(
            characterSettings);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->SetGravityFactor(0);
    virtualCharacter->SetCollisionLayerAndMask({0}, 0);
    auto* player = playerNode->AddObject<PlayerController>();

    auto* aimingAid = mainScene.CreateNode("AimingAid")->AddObject<AimingAid>();

    aimingAid->crosshair = GltfImporter::LoadScene(
        &mainScene, "./res/models/crosshair.glb", "crosshair", floorNode);
    aimingAid->crosshair->SetParent(aimingAid->GetNode());

    player->aim = aimingAid;

    player->AddObject<Player>();
    // player->GetObjectA<Player>()->SetRoomID(); default is 0

#pragma endregion

#pragma region Enemy
    /*JPH::BodyCreationSettings enemyShapeSettings = JPH::BodyCreationSettings(
        Physics::MeshShape(floorMeshRenderer->GetMesh()), JPH::RVec3::sZero(),
        JPH::Quat::sZero(), JPH::EMotionType::Dynamic,
       Physics::Layers::MOVING);*/
    ShaderProgram* pbrProg = ShaderProgram::Build()
                                 .WithVertexShader("./res/shaders/lit.vert")
                                 .WithPixelShader("./res/shaders/pbr.frag")
                                 .Link();

    Texture2D* reflectiveDiffuse = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-albedo.png",
        Texture::ColorTextureRGB);
    Texture2D* reflectiveNormal = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-Normal-ogl.png",
        Texture::TechnicalMapXYZ);
    Texture2D* reflectiveARM = mainScene.Resources()->Get<Texture2D>(
        "./res/textures/material_preview/worn-shiny-metal-arm.png",
        Texture::TechnicalMapXYZ);

    Material* reflectiveMat = new Material(pbrProg);
    reflectiveMat->SetValue("albedoMap", reflectiveDiffuse);
    reflectiveMat->SetValue("normalMap", reflectiveNormal);
    reflectiveMat->SetValue("armMap", reflectiveARM);

    auto enemyRoom = mainScene.FindNode("Floor");
    auto* surface = enemyRoom->GetObject<Surface>();
    SceneNode* enemy1 = mainScene.CreateNode("Enemy 1");
    /*Mesh* enemyMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/jake_tangents.glb");*/
    Material* enemyMat =
        mainScene.Resources()->Get<Material>("./res/materials/jake.mat");
    // enemy1->AddObject<MeshRenderer>(enemyMesh, reflectiveMat);
    enemy1->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, -5.0f);
    enemy1->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    // auto* enemyBody1 = enemy1->AddObject<Physics::Body>(enemyShapeSettings);
    JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.5f, 1.0f);
    JPH::BodyCreationSettings enemySettings(
        enemyShape, JPH::RVec3(10.5f, 2.0f, 2.0f), JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
    Physics::Body* enemyBody1 = enemy1->AddObject<Physics::Body>(enemySettings);
    enemyBody1->SetRestitution(0.0f);

    auto* enemyAi1 = enemy1->AddObject<EnemySkeleton>();

    enemyAi1->SetSurface(surface);
    // enemyAi1->GetSurface()->SetGroundHeight(0.0f);
    surface->AddEnemy(enemyAi1);
    enemyAi1->SetTargetNode(player->GetNode());
    surface->InformEnter(); // inform surface about player presence so it can
                            // assign the enemy to the correct room
    Mesh* cubeMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
    enemyAi1->SetProjectileResources(cubeMesh, enemyMat);
    enemyAi1->SetAttackCooldown(1.2f);
    enemyAi1->SetRoomID(1);
    // enemyAi1->DrawDebugView();

    SceneNode* enemyModel = GltfImporter::LoadScene(
        &mainScene, "./res/models/szkielet6.glb", "EnemyModel");
    enemyModel->SetParent(enemy1);
    enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);

    // Pobierz AnimationComponent z zaimportowanego modelu
    auto* animComp = enemyModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        spdlog::info(
            "Found AnimationComponent in enemy model, animations count: {}",
            animComp->animations.size());
    } else {
        spdlog::warn("No AnimationComponent found in enemy model");
    }

    if (animComp) {
        enemyAi1->SetAttackAnimation(animComp);
        // Opcjonalnie sprawd� dost�pne animacje i wybierz odpowiedni�
        // animComp->animations � lista dost�pnych animacji
    }

#pragma endregion
#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position());
    auto* fog = cameraNode->AddObject<Fog>();
    fog->fogType = Fog::Type::Atmospheric;
    fog->density = 0.042;
    fog->fogColor = {0.2, 0.6, 0.9, 1.0};
    cameraNode->AddObject<Bloom>();
    cameraNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    cameraNode->AddObject<ColorGrading>();
    cameraNode->AddObject<Fxaa>();

#pragma endregion
#pragma region Miscellaneous
    SceneNode* sun = mainScene.CreateNode("Sun");
    sun->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 2))
        ->SetShadowCasting(true);
    sun->GlobalTransform().Position() = {1, 2.2f, 0};
    sun->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(50.0f, -20.0f, 0.0f)));
    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        {1.0f, 1.0f, 1.0f, 0.6f});

    // Mesh* mirrorMesh =
    //     mainScene.Resources()->Get<Mesh>("./res/models/plane.obj");
    // SceneNode* mirrorNode = mainScene.CreateNode("Mirror");
    // SceneNode* mirrorMeshNode = mainScene.CreateNode(mirrorNode, "Mirror
    // Mesh"); mirrorMeshNode->AddObject<Mirror>(mirrorMesh);
    // mirrorNode->GlobalTransform().Position() = {15.0f, 0.0f, 1.5f};
    // mirrorNode->GlobalTransform().Rotation() =
    //     glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f)));
    // mirrorNode->GetObjectInChildren<MeshRenderer>()->GlobalTransform().Scale()
    // =
    //     {10.0f, 7.0f, 1.0f};
    // Physics::CreateCompoundShapeFromNode(
    //     mirrorNode->GetObjectInChildren<MeshRenderer>()->GetNode(), false,
    //     JPH::EMotionType::Static, Physics::Layers::NON_MOVING);

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFrequency(0.02f);
    Texture3D* noiseTexture = Noise3D::Create3DNoiseTexture(noise, 128, true);
    SceneNode* fogVolume = mainScene.CreateNode("Fog Volume");
    FogVolume* fogVolumeObject = fogVolume->AddObject<FogVolume>();
    fogVolumeObject->scatteringColor = {0.0f, 0.8f, 0.1f};
    fogVolumeObject->emissiveStrength = 5.0f;
    fogVolumeObject->stepSize = 0.03f;
    fogVolumeObject->scatteringDensity = 2.0f;
    fogVolumeObject->absorptionDensity = 0.0f;
    fogVolumeObject->noiseScale = 0.04f;
    fogVolumeObject->windDirection = {0.001f, 0.04f, 0.0f};
    fogVolumeObject->coverage = 0.4f;
    fogVolumeObject->sharpness = 6.0f;
    fogVolume->GlobalTransform().Position() = {0.0f, 0.6f, 3.0f};
    fogVolume->GlobalTransform().Scale() = {20.0f, 1.0f, 20.0f};
    fogVolumeObject->noiseTexture = noiseTexture;

    ShaderProgram* transparentProg =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/lit.vert")
            .WithPixelShader("./res/shaders/transparent.frag")
            .Link();

    Material* pinkTransparentMat = new Material(transparentProg);
    pinkTransparentMat->SetValue("uColor", glm::vec4(1.0, 0.5, 0.5, 0.6));

    Material* blueTransparentMat = new Material(transparentProg);
    blueTransparentMat->SetValue("uColor", glm::vec4(0.5, 0.5, 1.0, 0.6));

    // Mesh* cubeMesh =
    // mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

    SceneNode* pinkTransparentCubeNode = mainScene.CreateNode("Pink Cube");
    pinkTransparentCubeNode->AddObject<MeshRenderer>(cubeMesh,
                                                     pinkTransparentMat);
    pinkTransparentCubeNode->LocalTransform().Position() = {-3, 0, -3};

    SceneNode* blueTransparentCubeNode = mainScene.CreateNode("Blue Cube");
    blueTransparentCubeNode->AddObject<MeshRenderer>(cubeMesh,
                                                     blueTransparentMat);
    blueTransparentCubeNode->LocalTransform().Position() = {-3, 0, -5};

    ShaderProgram* scatterProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/scatter/scatter.vert")
            .WithPixelShader("./res/shaders/scatter/scatter.frag")
            .Link();
    auto scatterMaterial = new Material(scatterProgram);
    scatterMaterial->SetValue("uColor", glm::vec3(0.2, 0.6, 0.9));
    SceneNode* scatter = mainScene.CreateNode("Scatter");
    Scatter::Settings scatterSettings =
        Scatter::SettingsBuilder()
            .WithInstanceCount(1000)
            .WithAreaExtents(glm::vec3(50.0f, 50.0f, 50.0f))
            .AddProjection({.raycastLength = 20.0f, .raycastOffset = -20.0f})
            .AddTransform(
                {.minRotation = {glm::radians(-15.0f), 0.0f,
                                 glm::radians(-15.0f)},
                 .maxRotation = {glm::radians(15.0f), glm::radians(360.0f),
                                 glm::radians(15.0f)}})
            .Build();
    Scatter::Spawner* scatterSpawner = scatter->AddObject<Scatter::Spawner>(
        cubeMesh, std::move(scatterMaterial), scatterSettings);

    auto scatterMaterial2 = new Material(scatterProgram);
    scatterMaterial2->SetValue("uColor", glm::vec3(0.2, 0.1, 0.9));
    SceneNode* scatter2 = mainScene.CreateNode("Scatter");
    Scatter::Settings scatterSettings2 =
        Scatter::SettingsBuilder()
            .WithInstanceCount(100)
            .WithAreaExtents(glm::vec3(20.0f, 20.0f, 20.0f))
            .AddTransform(
                {.minRotation = {glm::radians(-15.0f), 0.0f,
                                 glm::radians(-15.0f)},
                 .maxRotation = {glm::radians(15.0f), glm::radians(360.0f),
                                 glm::radians(15.0f)}})
            .Build();
    Mesh* schnozMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");
    Scatter::Spawner* scatterSpawner2 = scatter2->AddObject<Scatter::Spawner>(
        schnozMesh, std::move(scatterMaterial2), scatterSettings2);

    ShaderProgram* dustProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/particles/particles.vert")
            .WithPixelShader("./res/shaders/particles/particles_blend.frag")
            .Link();

    auto dustMaterial = new Material(dustProgram);
    dustMaterial->SetValue("colorTex", mainScene.Resources()->Get<Texture2D>(
                                           "./res/textures/dust.png",
                                           Texture2D::ColorTextureRGBA));
    dustMaterial->SetValue("color", glm::vec4(200.0f, 200.0f, 200.0f, 1.0f));

    cameraNode->AddObject<ParticleSpawner>(
        mainScene.Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
        dustMaterial,
        ParticleSpawnerSettings{.maxParticles = 8192,
                                .areaExtents = glm::vec3(15.0f),
                                .emissionShapeExtents = glm::vec3(15.0f),
                                .minVelocity =
                                    glm::vec3(-0.08f, -0.05f, -0.08f),
                                .maxVelocity = glm::vec3(0.08f, 0.05f, 0.08f),
                                .minInitialAngle = 0.0f,
                                .maxInitialAngle = 6.28318f,
                                .minAngularVelocity = -0.2f,
                                .maxAngularVelocity = 0.2f,
                                .rotateY = false,
                                .enableLifetime = false,
                                .minLifetime = 1.0f,
                                .maxLifetime = 10000.0f,
                                .minScale = 0.02f,
                                .maxScale = 0.03f,
                                .alphaMode = AlphaMode::Alpha,
                                .enableProximityFade = true,
                                .proximityFadeMin = 0.2f,
                                .proximityFadeMax = 1.5f,
                                .enableDistanceFade = true,
                                .distanceFadeMin = 9.0f,
                                .distanceFadeMax = 12.0f,
                                .enableLifetimeFade = true,
                                .enableDepthFade = true,
                                .depthFadeDistance = 0.3f,
                                .billboardMode = BillboardMode::Enabled,
                                .wrapAround = true,
                                .continuous = false,
                                .useColorRamp = false});

#pragma endregion

    SceneNode* uiNode = mainScene.CreateNode("Ui Node");
    uiNode->AddObject<UiLayout>(glm::uvec2(200, 200), glm::uvec2(0, 0));
    uiNode->AddObject<UiVisual>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                mainScene.Resources()->Get<Texture2D>(
                                    "./res/textures/1147437805040054272.png",
                                    Texture2D::ColorTextureRGBA));

    return;
}
} // namespace TestScene
