#pragma once

#include "DepthOfField.h"
#include "GameObjectSystem.h"
#include "GltfImporter.h"
#include "JfaOutline.h"
#include "LightSystem.h"
#include "fog/Fog.h"
#include "game_scripts/AimingAid.h"
#include "ui/wheel/UiWheel.h"

#include <AiNode.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <DepthOfField.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <Light.h>
#include <MaskEffects.h>
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
#include <text/Font.h>
#include <ui/objects/UiCursor.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/objects/custom/UiCircularBar.h>
#include <ui/systems/UiSystem.h>
#include <ui/wheel/UiRadialWheel.h>

#include "Jolt/Math/Vec3.h"
#include "text/Text3D.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace TestScene {

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<AnimationSystem>();
    auto* tweenSystem = mainScene.AddComponent<TweenSystem>();
    mainScene.AddComponent<WheelSystem>();

    mainScene.GetGraphics()->ssaoSettings.enabled = false;

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

    auto floorNode =
        GltfImporter::LoadScene(&mainScene, "./res/models/floor.glb", "Floor");
    floorNode->AddObject<Skybox>(skyMat);
    MeshRenderer* floorMeshRenderer =
        floorNode->GetObjectInChildren<MeshRenderer>();
    floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});

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

#pragma endregion

#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position());
    cameraNode->AddObject<MaskEffects>();
    cameraNode->AddObject<JfaOutline>();
    auto* dof = cameraNode->AddObject<DepthOfField>();
    dof->SetEnabled(false);
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

    // Move this into the wheel system
    SceneNode* uiNode = mainScene.CreateNode("Ui Node");
    uiNode->AddObject<WheelTag>();
    uiNode->AddObject<UiLayout>(glm::uvec2(400, 400), glm::uvec2(150, 0), 0,
                                AnchorPoint::CenterLeft);
    auto* uiVisual = uiNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>(
            "./res/textures/1147437805040054272.png",
            Texture2D::ColorTextureRGBA));
    uiVisual->SetEnabled(false);
    uiVisual->colorHovered = {1.0f, 0.0f, 0.0f, 1.0f};
    uiVisual->colorClicked = {0.0f, 1.0f, 0.0f, 1.0f};
    uiNode->AddObject<UiInteractable>();

    SceneNode* cursorNode = mainScene.CreateNode("Cursor");
    cursorNode->AddObject<UiLayout>(glm::uvec2(64, 64), glm::uvec2(0, 0), 9999);

    cursorNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>(
            "./res/textures/1147437805040054272.png",
            Texture2D::ColorTextureRGBA));
    cursorNode->AddObject<UiCursor>();

    ShaderProgram* customUiProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/ui/ui.vert")
            .WithPixelShader("./res/shaders/ui/custom/radial_wheel.frag")
            .Link();
    Material* customUiMaterial = new Material(customUiProgram);
    SceneNode* radialWheelNode = mainScene.CreateNode("Radial Wheel");
    radialWheelNode->AddObject<UiLayout>(
        glm::uvec2(600, 600), glm::uvec2(-150, 0), 0, AnchorPoint::CenterRight);
    auto* customVisual =
        radialWheelNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    customVisual->customMaterial = customUiMaterial;
    auto* radialWheel = radialWheelNode->AddObject<UiRadialWheel>();
    radialWheel->AddObject<WheelTag>();
    radialWheel->material.reset(customUiMaterial);
    radialWheel->SetItemModels({
        "./res/models/butelka.glb",
        "./res/models/butelka.glb",
        "./res/models/butelka.glb",
        "./res/models/butelka.glb",
        "./res/models/butelka.glb",
    });
}
} // namespace TestScene
