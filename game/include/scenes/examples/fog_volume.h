#pragma once

#include "GltfScene.h"
#include "LightSystem.h"
#include "fog/Fog.h"
#include "game_scripts/AimCrosshair.h"

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
#include <ReflectionProbe.h>
#include <Scene.h>
#include <Shader.h>
#include <Skybox.h>
#include <TimeSystem.h>
#include <Tonemapper.h>
#include <TweenSystem.h>
#include <Viewport.h>
#include <animation/AnimationSystem.h>
#include <fog/FogVolume.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/enemies/EnemySkeleton.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/Helpers.h>
#include <physics/System.h>
#include <physics/VirtualCharacterController.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>
#include <text/Font.h>

#include "Jolt/Math/Vec3.h"
#include "ui/systems/UiSystem.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <game_scripts/enemies/EnemyBeetroot.h>
#include <game_scripts/enemies/MeleeSkeleton.h>
#include <imgui.h>
#include <memory>
#include <physics/VirtualCharacterController.h>

namespace ExampleFogVolume {

class Tornado : public GameObject, public ImGuiDrawable {
  private:
    Mesh* tornadoMesh;
    std::unique_ptr<ShaderProgram> tornadoShader;
    std::unique_ptr<Material> tornadoMaterial;
    SceneNode* tornadoNode;

    Mesh* particlesMesh;
    std::unique_ptr<ShaderProgram> particlesShader;
    std::unique_ptr<Material> particlesBottomMaterial;
    std::unique_ptr<Material> particlesTopMaterial;
    SceneNode* particlesBottom;
    SceneNode* particleTop;

    float rotationSpeed = 10.0f;
    float opacity = 0.7f;
    float intensity = 5.5f;
    glm::vec2 uvSpeed = {2.0f, 1.0f};

  public:
    Tornado() {
        this->tornadoMesh = this->GetScene()->Resources()->Get<Mesh>(
            "./res/models/tornado/tornado.obj");
        this->tornadoShader.reset(
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/lit.vert")
                .WithPixelShader("./res/shaders/tornado.frag")
                .Link());
        this->tornadoMaterial =
            std::make_unique<Material>(this->tornadoShader.get());
        this->tornadoMaterial->SetValue("uOpacity", this->opacity);
        this->tornadoMaterial->SetValue("uSpeed", this->uvSpeed);
        this->tornadoMaterial->SetValue("uIntensity", this->intensity);

        this->tornadoMaterial->SetValue(
            "uNoiseTexture", this->GetScene()->Resources()->Get<Texture2D>(
                                 "./res/textures/noise/T_Noise_HU85k.png",
                                 Texture::TechnicalMapXYZ));
        this->tornadoMaterial->SetValue(
            "uColorGradientTexture",
            this->GetScene()->Resources()->Get<Texture2D>(
                "./res/textures/inkpink-alpha.png", Texture::ColorTextureRGBA));
        this->tornadoMaterial->SetValue(
            "uSubtractionTexture",
            this->GetScene()->Resources()->Get<Texture2D>(
                "./res/textures/subtraction.png", Texture::TechnicalMapXYZ));

        this->tornadoNode =
            this->GetScene()->CreateNode(this->GetNode(), "Tornado");
        this->tornadoNode->AddObject<MeshRenderer>(this->tornadoMesh,
                                                   this->tornadoMaterial.get());

        this->particlesBottom = this->GetScene()->CreateNode(
            this->GetNode(), "Tornado Particles Bottom");
        this->particlesMesh = this->GetScene()->Resources()->Get<Mesh>(
            "./res/models/tornado/tornado_base.obj");
        this->particlesShader.reset(
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/particles/particles.vert")
                .WithPixelShader("./res/shaders/particles/tornado_base.frag")
                .Link());

        this->particlesBottomMaterial =
            std::make_unique<Material>(this->particlesShader.get());
        this->particlesTopMaterial =
            std::make_unique<Material>(this->particlesShader.get());

        this->particlesBottomMaterial->SetValue(
            "colorTex", this->GetScene()->Resources()->Get<Texture2D>(
                            "./res/textures/noise/T_FirePanningCyl45.png",
                            Texture::TechnicalMapXYZ));

        Texture2D* colorRampTex = this->GetScene()->Resources()->Get<Texture2D>(
            "./res/textures/inkpink-32x.png", Texture::ColorTextureRGB);
        this->particlesBottomMaterial->SetValue("colorRamp", colorRampTex);

        ParticleSpawnerSettings particleSettings = {
            .maxParticles = 8,
            .emissionShapeExtents = glm::vec3(0.01f),

            .minVelocity = {0.0f, 0.0f, 0.0f},
            .maxVelocity = {0.0f, 0.2f, 0.0f},

            .minInitialAngle = glm::radians(-180.0f),
            .maxInitialAngle = glm::radians(180.0f),
            .minAngularVelocity = -4.0f,
            .maxAngularVelocity = 8.0f,
            .rotateY = true,

            .enableLifetime = true,
            .minLifetime = 2.0f,
            .maxLifetime = 3.0f,

            .minScale = 1.5f,
            .maxScale = 1.8f,

            .alphaMode = AlphaMode::Alpha,
            .lifetimeFadeIn = {0.0f, 0.1f},
            .lifetimeFadeOut = {0.9f, 1.0f},

            .wrapAround = false,
            .continuous = true,
            .useColorRamp = true,
        };

        this->particlesBottom->AddObject<ParticleSpawner>(
            this->particlesMesh, particlesBottomMaterial.get(),
            particleSettings);

        this->particleTop = this->GetScene()->CreateNode(
            this->GetNode(), "Tornado Particles Top");
        this->particlesTopMaterial->SetValue(
            "colorTex", this->GetScene()->Resources()->Get<Texture2D>(
                            "./res/textures/noise/T_FirePanningCyl45.png",
                            Texture::TechnicalMapXYZ));
        this->particlesTopMaterial->SetValue("colorRamp", colorRampTex);

        particleSettings.minScale = 3.5f;
        particleSettings.maxScale = 4.0f;
        this->particleTop->AddObject<ParticleSpawner>(
            this->particlesMesh, particlesTopMaterial.get(), particleSettings);
        this->particleTop->LocalTransform().Position() = {0.0f, 4.8f, 0.0f};
    }

    void Update() {
        glm::quat rotation = glm::angleAxis(glm::radians(this->rotationSpeed),
                                            glm::vec3(0.0f, 1.0f, 0.0f));

        this->tornadoNode->LocalTransform().Rotation() *= rotation;
    }

    virtual void DrawImGui() {
        ImGui::SliderFloat("Rotation Speed", &this->rotationSpeed, 0.0f, 50.0f);
        if (ImGui::SliderFloat("Opacity", &this->opacity, 0.0f, 1.0f)) {
            this->tornadoMaterial->SetValue("uOpacity", this->opacity);
        }
        if (ImGui::InputFloat2("UV Speed", &this->uvSpeed[0])) {
            this->tornadoMaterial->SetValue("uSpeed", this->uvSpeed);
        }
        if (ImGui::SliderFloat("Intensity", &this->intensity, 0.0f, 10.0f)) {
            this->tornadoMaterial->SetValue("uIntensity", this->intensity);
        }
    }
};

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
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<AnimationSystem>();

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

    auto floorNode = mainScene.Resources()
                         ->Get<GltfScene>("./res/models/floor.glb")
                         ->Instantiate(&mainScene, nullptr, "Floor");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});
    auto* surface =
        floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 1.0f);

#pragma endregion
#pragma region Player

    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
        new JPH::CharacterVirtualSettings();
    characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
    characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    SceneNode* playerNode = mainScene.CreateNode("Player");

    SceneNode* bimberman =
        ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/bimbermann_throwing.glb")
            ->Instantiate(&mainScene, mainScene.root, "Bimberman");
    bimberman->SetParent(playerNode);

    auto* virtualCharacter =
        playerNode->AddObject<Physics::VirtualCharacterController>(
            characterSettings);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->SetGravityFactor(0);
    virtualCharacter->SetCollisionLayerAndMask({0}, 0);

    auto* aimingAid =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/crosshair.glb")
            ->Instantiate(&mainScene, playerNode, "Aim Reticle")
            ->AddObject<AimCrosshair>();

    auto* player = playerNode->AddObject<PlayerController>();

#pragma endregion

#pragma region Enemy
    /*JPH::BodyCreationSettings enemyShapeSettings = JPH::BodyCreationSettings(
        Physics::MeshShape(floorMeshRenderer->GetMesh()), JPH::RVec3::sZero(),
        JPH::Quat::sZero(), JPH::EMotionType::Dynamic,
       Physics::Layers::MOVING);*/

    JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.5f, 1.0f);
    JPH::BodyCreationSettings enemySettings(
        enemyShape, JPH::RVec3(10.5f, 2.0f, 2.0f), JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
    Material* enemyMat =
        mainScene.Resources()->Get<Material>("./res/materials/jake.mat");
    Mesh* cubeMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");

    SceneNode* enemy1 = mainScene.CreateNode("Enemy 1");
    enemy1->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    Physics::Body* enemyBody1 = enemy1->AddObject<Physics::Body>(enemySettings);
    enemyBody1->SetRestitution(0.0f);
    auto* enemyAi1 = enemy1->AddObject<MeleeSkeleton>();
    enemyAi1->SetSurface(surface);
    enemyAi1->SetTargetNode(player->GetNode());
    enemyAi1->SetProjectileResources(cubeMesh, enemyMat);
    enemyAi1->SetAttackCooldown(1.2f);
    enemyAi1->SetRoomID(1);

    SceneNode* enemyModel =
        mainScene.resources.Get<GltfScene>("./res/models/szkielet6.glb")
            ->Instantiate(&mainScene, enemy1, "EnemyModel");
    enemyModel->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    enemy1->GlobalTransform().Position() = glm::vec3(10.5f, 0.0f, -5.0f);
    enemyModel->LocalTransform().Position() = glm::vec3(0);

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
    }

    // SceneNode* enemy2 = mainScene.CreateNode("Enemy 2");
    // //enemy2->GlobalTransform().Position() = glm::vec3(12.0f, 0.0f, -5.0f);
    // enemy2->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    // Physics::Body* enemyBody2 =
    // enemy2->AddObject<Physics::Body>(enemySettings);
    // enemyBody2->SetRestitution(0.0f);
    // auto* enemyAi2 = enemy2->AddObject<EnemyBeetroot>();
    // enemyAi2->SetSurface(surface);
    // enemyAi2->SetTargetNode(player->GetNode());
    // enemyAi2->SetProjectileResources(cubeMesh, enemyMat);
    // enemyAi2->SetAttackCooldown(1.2f);
    // enemyAi2->SetRoomID(1);

    // SceneNode* enemyModel2 =
    // mainScene.resources.Get<GltfScene>("./res/models/enemies/ziemniak_remake.glb")->Instantiate(&mainScene,
    // enemy2, "EnemyModel2");
    // //enemyModel2->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    // auto* animComp2 = enemyModel2->GetObjectInChildren<AnimationComponent>();
    // if (animComp2) {
    //     spdlog::info(
    //         "Found AnimationComponent in enemy model 2, animations count:
    //         {}", animComp2->animations.size());
    // } else {
    //     spdlog::warn("No AnimationComponent found in enemy model 2");
    // }
    // if (animComp2) {
    //     enemyAi2->SetAttackAnimation(animComp2);
    // }

    // SceneNode* enemy3 = mainScene.CreateNode("Enemy 3");
    // //enemy3->GlobalTransform().Position() = glm::vec3(13.5f, 0.0f, -5.0f);
    // //enemy3->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    // Physics::Body* enemyBody3 =
    // enemy3->AddObject<Physics::Body>(enemySettings);
    // enemyBody3->SetRestitution(0.0f);
    // auto* enemyAi3 = enemy3->AddObject<EnemyPotato>();
    // enemyAi3->SetSurface(surface);
    // enemyAi3->SetTargetNode(player->GetNode());
    // enemyAi3->SetProjectileResources(cubeMesh, enemyMat);
    // enemyAi3->SetAttackCooldown(1.2f);
    // enemyAi3->SetRoomID(1);

    // SceneNode* enemyModel3 = GltfImporter::LoadScene(
    //     &mainScene, "./res/models/enemies/burak_macki2.glb", "EnemyModel3");
    // enemyModel3->SetParent(enemy3);
    // //enemyModel3->GlobalTransform().Scale() = glm::vec3(0.1, 0.1, 0.1);
    // auto* animComp3 = enemyModel3->GetObjectInChildren<AnimationComponent>();
    // if (animComp3) {
    //     spdlog::info(
    //         "Found AnimationComponent in enemy model 3, animations count:
    //         {}", animComp3->animations.size());
    // } else {
    //     spdlog::warn("No AnimationComponent found in enemy model 3");
    // }
    // if (animComp3) {
    //     enemyAi3->SetAttackAnimation(animComp3);
    // }

    surface->AddEnemy(enemyAi1);
    // surface->AddEnemy(enemyAi2);
    // surface->AddEnemy(enemyAi3);
    surface->InformEnter();

#pragma endregion
#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position());
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

#pragma endregion

    SceneNode* fogNode = mainScene.CreateNode("Fog Volume");
    fogNode->GlobalTransform().Position() = glm::vec3(0.0, 0.5f, 0.0f);
    fogNode->GlobalTransform().Scale() = glm::vec3(30.0f, 1.0f, 30.0f);

    auto* fluidSim = fogNode->AddObject<FluidSimulation>();
    auto* fogVolume = fogNode->AddObject<FogVolume>();

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.05f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);

    float noiseTexSize = 64.0f;
    Texture3D* generatedNoise =
        Noise3D::Create3DNoiseTexture(noise, noiseTexSize, true);

    fogVolume->noiseTexture = generatedNoise;
    fogVolume->stepSize = 0.015f;
    fogVolume->scatteringDensity = 2.0f;
    fogVolume->absorptionDensity = 2.0f;
    fogVolume->scatteringColor = {124.0f / 255.0f, 0.0f, 1.0f};
    fogVolume->emissiveStrength = 0.784;
    fogVolume->noiseScale = 0.090f;
    fogVolume->velocityTexture = fluidSim->GetVelocityMap();
    fogVolume->velocityStrength = 5.0f;

    fluidSim->damping = 0.948f;
    fluidSim->playerRadius = 0.033f;
    fluidSim->interactionStrength = 0.840f;
}
} // namespace ExampleFogVolume
