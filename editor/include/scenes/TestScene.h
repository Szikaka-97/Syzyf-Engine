#pragma once

#include "DepthOfField.h"
#include "GameObjectSystem.h"
#include "GltfImporter.h"
#include "JfaOutline.h"
#include "LightSystem.h"
#include "fog/Fog.h"
#include "game_scripts/player/PickableItemSystem.h"
#include "ui/custom/UiHealthBar.h"
#include "ui/custom/wheel/UiWheel.h"
#include <game_scripts/player/AimingAid.h>
#include <game_scripts/player/Player.h>
#include <game_scripts/player/PlayerController.h>

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
#include <game_scripts/player/CameraSettings.h>
#include <game_scripts/player/PlayerController.h>
#include <game_scripts/player/ThrowBottle.h>
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
#include <ui/custom/UiCircularBar.h>
#include <ui/custom/wheel/UiRadialWheel.h>
#include <ui/objects/UiCursor.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiScrollableGrid.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/systems/UiSystem.h>

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
    mainScene.AddComponent<PickableItemSystem>();
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
    auto* floorBody = floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});

    floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

    auto* room = GltfImporter::LoadScene(
        &mainScene, "./res/models/rooms/room_I.gltf", "Room");
    room->GlobalTransform().Position() += {0.0f, 0.01f, 0.0f};

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
    virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->SetGravityFactor(1);
    auto* player = playerNode->AddObject<PlayerController>();

    auto* aimingAid = mainScene.CreateNode("AimingAid")->AddObject<AimingAid>();

    aimingAid->crosshair = GltfImporter::LoadScene(
        &mainScene, "./res/models/crosshair.glb", "crosshair", floorNode);
    aimingAid->crosshair->SetParent(aimingAid->GetNode());
    aimingAid->GetNode()->GetObjectInChildren<MeshRenderer>()->maskFlags |=
        MaskEffectBits::XRay;

    player->aim = aimingAid;

    player->AddObject<Player>();

    // Pickable objects
    auto* schnozMesh = mainScene.Resources()->Get<Mesh>(
        "./res/models/schnoz/schnoz.obj", true);
    JPH::ShapeRefC schnozShape = Physics::ConvexHullMeshShape(schnozMesh);
    JPH::BodyCreationSettings schnozSettings(
        schnozShape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic, Physics::Layers::MOVING);

    for (int i = 0; i < 10; i++) {
        auto* item = mainScene.CreateNode("PickableSchnoz");
        item->AddObject<MeshRenderer>(schnozMesh,
                                      schnozMesh->GetDefaultMaterials());
        item->AddObject<PickableItem>();
        auto* itemBody = item->AddObject<Physics::Body>(schnozSettings);
        itemBody->SetCollisionLayerAndMask({2}, 0xFFFFFFFF);

        item->GlobalTransform().Scale() = glm::vec3(0.2f);
        item->GlobalTransform().Position() = {2.0f, 2.0f + i * 0.5f, 2.0f};
    }

#pragma endregion

#pragma region Enemy

    SceneNode* enemy =
        GltfImporter::LoadScene(&mainScene, "./res/models/szkielet6.glb");
    enemy->GlobalTransform().Position() = {3.0f, 1.0f, 0.0f};
    enemy->GlobalTransform().Scale() = glm::vec3(0.1f);

#pragma endregion

#pragma region Camera

    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position());
    cameraNode->AddObject<MaskEffects>();
    auto* jfa = cameraNode->AddObject<JfaOutline>();
    jfa->outlineThickness = 4.0f;
    jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};
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
    SceneNode* uiRoot = mainScene.CreateNode("UI");

    // Move this into the wheel system
    SceneNode* uiNode = mainScene.CreateNode(uiRoot, "Ui Node");
    uiNode->AddObject<WheelTag>();
    uiNode->AddObject<UiLayout>(glm::uvec2(400, 400), glm::uvec2(150, 0), 0,
                                AnchorPoint::CenterLeft);
    uiNode->AddObject<UiInteractable>();

    SceneNode* cursorNode = mainScene.CreateNode(uiRoot, "Cursor");
    cursorNode->AddObject<UiLayout>(glm::uvec2(64, 64), glm::uvec2(0, 0), 9999);

    cursorNode->AddObject<UiVisual>(
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        mainScene.Resources()->Get<Texture2D>("./res/textures/cursor.png",
                                              Texture2D::ColorTextureRGBA));
    cursorNode->AddObject<UiCursor>();

    ShaderProgram* customUiProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/ui/ui.vert")
            .WithPixelShader("./res/shaders/ui/custom/radial_wheel.frag")
            .Link();
    Material* customUiMaterial = new Material(customUiProgram);
    SceneNode* radialWheelNode = mainScene.CreateNode(uiRoot, "Radial Wheel");
    radialWheelNode->AddObject<UiLayout>(
        glm::uvec2(600, 600), glm::uvec2(-50, 0), 0, AnchorPoint::CenterRight);
    auto* customVisual =
        radialWheelNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    customVisual->SetEnabled(false);
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

    SceneNode* gridRoot = mainScene.CreateNode(uiRoot, "Grid");
    gridRoot->AddObject<UiLayout>(glm::uvec2(360, 240), glm::uvec2(50, 300), 0,
                                  AnchorPoint::TopLeft);
    auto* gridRootVisual =
        gridRoot->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
    gridRootVisual->SetEnabled(false);
    gridRoot->AddObject<WheelTag>();
    SceneNode* gridContainer = mainScene.CreateNode(gridRoot, "Grid Container");
    auto* gridLayout = gridContainer->AddObject<UiLayout>(
        glm::uvec2(330, 210), glm::uvec2(0, 0), 0, AnchorPoint::Center);
    gridContainer->AddObject<UiInteractable>();

    auto* grid = gridContainer->AddObject<UiScrollableGrid>();

    // Up Button
    SceneNode* gridUpButton = mainScene.CreateNode(gridRoot, "Grid Up Button");
    gridUpButton->AddObject<UiLayout>(glm::uvec2(125, 70), glm::uvec2(115, -80),
                                      3, AnchorPoint::TopLeft);
    gridUpButton->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 0.8f))
        ->SetEnabled(false);
    gridUpButton->AddObject<WheelTag>();
    // scary
    gridUpButton->AddObject<UiInteractable>()->OnClick = [grid]() {
        grid->ScrollUp();
    };

    // Down Button
    SceneNode* gridDownButton =
        mainScene.CreateNode(gridRoot, "Grid Down Button");
    gridDownButton->AddObject<UiLayout>(
        glm::uvec2(125, 70), glm::uvec2(115, 250), 3, AnchorPoint::TopLeft);
    gridDownButton->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 0.8f))
        ->SetEnabled(false);
    gridDownButton->AddObject<WheelTag>();
    gridDownButton->AddObject<UiInteractable>()->OnClick = [grid]() {
        grid->ScrollDown();
    };

    for (int i = 0; i < 20; i++) {
        SceneNode* itemNode =
            mainScene.CreateNode(uiRoot, "Item_" + std::to_string(i));
        itemNode->SetParent(gridContainer);

        auto* layout = itemNode->AddObject<UiLayout>(
            glm::uvec2(100, 100), glm::uvec2(0, 0), 1, AnchorPoint::Center);

        auto* visual =
            itemNode->AddObject<UiVisual>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        visual->SetEnabled(false);
        itemNode->AddObject<UiInteractable>();
        itemNode->AddObject<WheelTag>();
    }

    SceneNode* healthBarOutline =
        mainScene.CreateNode(uiRoot, "HealthBar Outline");
    auto* healthBarOutlineLayout = healthBarOutline->AddObject<UiLayout>(
        glm::uvec2(365, 60), glm::uvec2(50, 50), 0, AnchorPoint::TopLeft);
    healthBarOutline->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));

    // Health bar
    SceneNode* healthBarBackground =
        mainScene.CreateNode(healthBarOutline, "HealthBar Background");
    auto* healthBarBackgroundLayout = healthBarBackground->AddObject<UiLayout>(
        glm::uvec2(335, 30), glm::uvec2(0, 0), 1, AnchorPoint::Center);
    auto* bgVisual = healthBarBackground->AddObject<UiVisual>(
        glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));

    SceneNode* healthBarFill =
        mainScene.CreateNode(healthBarBackground, "HealthBar Fill");
    auto* healthBarFillLayout = healthBarFill->AddObject<UiLayout>(
        glm::uvec2(335, 30), glm::uvec2(0, 0), 2, AnchorPoint::CenterLeft);
    auto* fillVisual =
        healthBarFill->AddObject<UiVisual>(glm::vec4(0.8f, 0.1f, 0.1f, 1.0f));

    auto* healthBarLogic = healthBarOutline->AddObject<UiHealthBar>();
    healthBarLogic->fillLayout = healthBarFillLayout;
    healthBarLogic->maxWidth = 335;
    healthBarLogic->playerNode = playerNode;
    healthBarLogic->mockEnemyNode = enemy;
    healthBarLogic->fillVisual = fillVisual;
    healthBarLogic->bgVisual = bgVisual;
}
} // namespace TestScene
