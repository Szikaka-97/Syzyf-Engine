#pragma once

#include "EasingFunctions.h"
#include "GltfImporter.h"
#include "LightSystem.h"
#include "game_scripts/ThrowBottle.h"

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
#include <game_scripts/ThrowBottle.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <physics/Body.h>
#include <physics/DebugRenderer.h>
#include <physics/System.h>
#include <physics/Water.h>
#include <scatter/Spawner.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace TestScene {
class EditorCameraTag : public GameObject {};

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
    mainScene.AddComponent<AnimationSystem>();
    auto* tweenSystem = mainScene.AddComponent<TweenSystem>();

    ShaderProgram* skyProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/skybox.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/skybox.frag"))
            .Link();

    ShaderProgram* coloredProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/lambert color.frag"))
            .Link();

    ShaderProgram* diffuseTexProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/lambert.frag"))
            .Link();

    ShaderProgram* pbrProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/pbr.frag"))
            .Link();

    ShaderProgram* pbrRefractProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/pbr refract.frag"))
            .Link();

    ShaderProgram* transparentProg =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/lit.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/transparent.frag"))
            .Link();
    transparentProg->SetTransparent(true);

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/citrus_orchard_road_puresky.hdr",
        Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

    // ---- PLAYER ----
    SceneNode* playerNode = mainScene.CreateNode("Player");
    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    playerNode->GlobalTransform().Position() = glm::vec3(0.0f, 5.0f, 0.0f);
    cameraNode->AddObject<Camera>(
        Camera::Perspective(25.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(playerNode);
    playerNode->AddObject<Bloom>();
    playerNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    playerNode->AddObject<ColorGrading>();
    playerNode->AddObject<Fxaa>();

    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
        new JPH::CharacterVirtualSettings();
    characterSettings->mShape = new JPH::CapsuleShape(1.0f, 0.5f);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    auto* virtualCharacter =
        playerNode->AddObject<Physics::VirtualCharacterController>(
            characterSettings);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->Awake();

    auto mouseMarkerNode = mainScene.CreateNode("Mouse Marker");
    mouseMarkerNode->GlobalTransform().Scale() = glm::vec3(0.15f, 0.02f, 0.15f);

    auto* bottleThrower = playerNode->AddObject<ThrowBottle>();
    bottleThrower->SetPoolSize(10);
    auto* controller = playerNode->AddObject<PlayerController>(mouseMarkerNode);
    controller->SetBottleThrower(bottleThrower);

    Mesh* schnozMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/schnoz/schnoz.obj");
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

    SceneNode* playerMeshNode = mainScene.CreateNode(playerNode);
    playerMeshNode->AddObject<MeshRenderer>(schnozMesh, reflectiveMat);
    playerMeshNode->GlobalTransform().Position() = glm::vec3(0.0f, 2.5f, 0.0f);
    playerMeshNode->GlobalTransform().Scale() = glm::vec3(0.5f, 0.5f, 0.5f);
    Mesh* cubeMesh =
        mainScene.Resources()->Get<Mesh>("./res/models/not_cube.obj");
    bottleThrower->SetResources(cubeMesh, reflectiveMat);

    auto floorNode =
        GltfImporter::LoadScene(&mainScene, "./res/models/floor.glb", "Floor");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    floorMeshRenderer->GetNode()->AddObject<Physics::Body>(Physics::Body::Mesh(
        floorMeshRenderer->GetMesh(), JPH::EMotionType::Static,
        Physics::Layers::NON_MOVING));
    floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 1.0f);

    SceneNode* monkey = GltfImporter::LoadScene(
        &mainScene, "./res/models/big_monkey.glb", "Monkey", floorNode);
    JPH::ShapeRefC monkeyShape = Physics::CreateCompoundShapeFromNode(
        monkey, false, JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
    monkey->AddObject<Physics::Body>(JPH::BodyCreationSettings{
        monkeyShape, JPH::Vec3::sZero(), JPH::Quat::sIdentity(),
        JPH::EMotionType::Static, Physics::Layers::MOVING});

    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        {1.0f, 1.0f, 1.0f, 0.8f});

    auto lightNode = mainScene.CreateNode("Point Light");
    lightNode->AddObject<Light>(Light::PointLight({1, 1, 1}, 10, 1))
        ->SetShadowCasting(true);
    lightNode->GlobalTransform().Position() = {-1, 2.2f, 0};

    auto lightNode2 = mainScene.CreateNode("Directional Light");
    lightNode2->AddObject<Light>(Light::DirectionalLight({1, 1, 1}, 1))
        ->SetShadowCasting(true);
    lightNode2->GlobalTransform().Position() = {1, 2.2f, 0};
    lightNode2->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(64.0f, 0.0f, 0.0f)));

    auto envProbe2 = mainScene.CreateNode("Reflection Probe");
    envProbe2->AddObject<ReflectionProbe>();
    envProbe2->GlobalTransform().Position() = {-10.0f, 1.5f, 0.6f};

    auto envProbe3 = mainScene.CreateNode("Reflection Probe");
    envProbe3->AddObject<ReflectionProbe>();
    envProbe3->GlobalTransform().Position() = {-29.0f, 1.5f, 0.6f};

    SceneNode* skeletonNode = GltfImporter::LoadScene(
        &mainScene, "./res/models/szkielet6.glb", "Szkielet");
    skeletonNode->GlobalTransform().Scale() = glm::vec3(0.2f);

    SceneNode* skeleton2Node = GltfImporter::LoadScene(
        &mainScene, "./res/models/szkielet6.glb", "Szkielet2");
    skeleton2Node->GlobalTransform().Position() = {0.0f, 0.0f, 5.0f};
    skeleton2Node->GlobalTransform().Scale() = glm::vec3(0.2f);

    SceneNode* bimberman = GltfImporter::LoadScene(
        &mainScene, "./res/models/bimbermann.glb", "Bimberman");
    bimberman->GlobalTransform().Scale() = glm::vec3(5.0f);
    bimberman->GlobalTransform().Rotation() =
        glm::quat(glm::radians(glm::vec3(0.0f, 90.0f, 0.0f)));
    bimberman->GlobalTransform().Position() = {0.0f, 0.0f, 10.0f};

    ShaderProgram* scatterProgram =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/scatter.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/lambert color.frag"))
            .Link();
    scatterProgram->SetCastsShadows(false);
    scatterProgram->SetIgnoresDepthPrepass(true);
    auto scatterMaterial = std::make_unique<Material>(scatterProgram);
    scatterMaterial->SetValue("uColor", glm::vec3(0.2, 0.6, 0.9));
    SceneNode* scatter = mainScene.CreateNode("Scatter");
    Scatter::Settings scatterSettings =
        Scatter::SettingsBuilder()
            .WithInstanceCount(5000)
            .WithAreaExtents(glm::vec3(50.0f, 0.0f, 50.0f))
            .AddProjection({.raycastLength = 20.0f, .raycastOffset = 20.0f})
            .AddRelax({.minDistance = 2.0f, .maxAttempts = 30})
            .AddTransform(
                {.minRotation = {glm::radians(-15.0f), 0.0f,
                                 glm::radians(-15.0f)},
                 .maxRotation = {glm::radians(15.0f), glm::radians(360.0f),
                                 glm::radians(15.0f)}})
            .AddArray({.arraySize = 0})
            .AddArray({.arraySize = 1})
            .Build();
    Scatter::Spawner* scatterSpawner = scatter->AddObject<Scatter::Spawner>(
        cubeMesh, std::move(scatterMaterial), scatterSettings);

    ShaderProgram* dustProgram =
        ShaderProgram::Build()
            .WithVertexShader(mainScene.Resources()->Get<VertexShader>(
                "./res/shaders/particles/particles.vert"))
            .WithPixelShader(mainScene.Resources()->Get<PixelShader>(
                "./res/shaders/particles/particles_dither.frag"))
            .Link();
    dustProgram->SetTransparent(false);
    dustProgram->SetCastsShadows(false);

    auto dustMaterial = std::make_unique<Material>(dustProgram);
    dustMaterial->SetValue("colorTex", mainScene.Resources()->Get<Texture2D>(
                                           "./res/textures/dust.png",
                                           Texture2D::ColorTextureRGBA));
    dustMaterial->SetValue("color", glm::vec4(200.0f, 200.0f, 200.0f, 1.0f));

    playerNode->AddObject<ParticleSpawner>(
        mainScene.Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
        std::move(dustMaterial),
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
                                .alphaMode = AlphaMode::Dither,
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

    floorNode->AddObject<Surface>(floorMeshRenderer->GetMesh(), 10.0f);
    // AStarManager::Instance().BuildGraph(floorNode->GetObject<Surface>(), 10.0f);
    auto* navGrid = floorNode->AddObject<NavigationGrid>();
    navGrid->Build(floorNode->GetObject<Surface>(), 2.0f, 45.0f);

    SceneNode* w_schnozNode = mainScene.CreateNode("w_schnozNode");
    w_schnozNode->LocalTransform().Position() = glm::vec3(-20, 0, -20);
    // schnozNode->LocalTransform().Scale() = glm::vec3(1, 1, 1);
    w_schnozNode->AddObject<MeshRenderer>(schnozMesh, reflectiveMat);

    JPH::BodyCreationSettings w_schnozShapeSettings =
        Physics::Body::ConvexHullMesh(schnozMesh, JPH::EMotionType::Dynamic,
                                      Physics::Layers::MOVING);
    auto* w_schnozBody =
        w_schnozNode->AddObject<Physics::Body>(w_schnozShapeSettings);
    w_schnozBody->SetRestitution(0.0f);
    w_schnozBody->SetFriction(0.5f);
    w_schnozBody->SetLinearDamping(0.1f);
    // w_schnozBody->Awake();
    // w_schnozBody->SetCollisionLayerAndMask({ 0 });
    w_schnozBody->SetCollisionLayerAndMask(
        {Physics::Layers::MOVING, Physics::Layers::NON_MOVING});

    auto enemyAI = w_schnozNode->AddObject<AiNode>();
    if (enemyAI) {
        enemyAI->SetTarget(playerNode);
        enemyAI->SetProjectileResources(
            cubeMesh,
            reflectiveMat); // u�yj istniej�cych zasob�w <-- :raised_eyebrow:?
        enemyAI->SetAttackCooldown(1.2f);
    }

    glm::vec2 patrolPoints[] = {glm::vec2(-20, 0), glm::vec2(-40, 0)};

    /*auto aiNode = w_schnozNode->GetObject<AiNode>();
    if (aiNode) {
            aiNode->SetPatrolPoints(patrolPointsVec);
    }*/

    std::vector<glm::vec2> patrolPointsVec(std::begin(patrolPoints),
                                           std::end(patrolPoints));
    w_schnozNode->GetObject<AiNode>()->SetPatrolPoints(patrolPointsVec);

    SceneNode* schnozLightNode = mainScene.CreateNode("Schnoz Light");
    schnozLightNode->LocalTransform().Position() = glm::vec3(-55.5, 3.0, -2.0);
    schnozLightNode->AddObject<Light>(
        Light::PointLight(glm::vec3(1, 1, 1), 5, 5));

    tweenSystem->CreateTween({0.0f, 1.0f, 15.0f, Easing::outBounce})
        .Bind([bimberman](float value) {
            bimberman->LocalTransform().Scale() =
                glm::vec3(value) * 10.0f + value * 10.0f;
            bimberman->LocalTransform().Rotation() =
                glm::quat(glm::radians(glm::vec3(0.0f, 360.0f * value, 0.0f)));
        })
        .Detach();
    ;
}
} // namespace TestScene
