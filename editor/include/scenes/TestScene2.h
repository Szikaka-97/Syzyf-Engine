#pragma once

#include "GltfImporter.h"
#include "LightSystem.h"
#include "fog/Fog.h"
#include "game_scripts/AimingAid.h"

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
#include <enemies/EnemySkeleton.h>
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

#include "Jolt/Math/Vec3.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace TestScene2 {

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
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

    return;
}
} // namespace TestScene2
