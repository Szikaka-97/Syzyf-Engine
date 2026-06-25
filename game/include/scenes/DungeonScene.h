#pragma once

#include "DepthOfField.h"
#include "EasingFunctions.h"
#include "GltfScene.h"
#include "LightSystem.h"
#include <game_scripts/DungeonGenerator.h>

#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <Framebuffer.h>
#include <Fxaa.h>
#include <InputSystem.h>
#include <JfaOutline.h>
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
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/GameAudio.h>
// #include <game_scripts/ThrowBottle.h>
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
#include "game_scripts/enemies/FlockingSystem.h"
#include "game_scripts/ui/InGame.h"
#include "game_scripts/ui/PauseMenu.h"
#include "game_scripts/ui/TabMenu.h"
#include "ui/systems/UiSystem.h"
#include "ui/widgets/wheel/UiWheel.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <game_scripts/AimCrosshair.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/ui/PauseMenu.h>
#include <ui/systems/UiSystem.h>
#include <game_scripts/enemies/FlockingSystem.h>
#include <imgui.h>
#include <physics/VirtualCharacterController.h>

namespace DungeonScene {
inline void InitScene(Scene& mainScene) {
	mainScene.AddComponent<Physics::System>();
	mainScene.AddComponent<DebugInspector>();
	mainScene.AddComponent<UiSystem>();
	mainScene.AddComponent<AnimationSystem>();
	auto* tweenSystem = mainScene.AddComponent<TweenSystem>();
    mainScene.AddComponent<PickableItemSystem>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<WheelSystem>();
    mainScene.AddComponent<ThrowableObjectPool>();
    GameAudio::AddLooping2D(
        mainScene,
        "Dungeon Ambient Audio",
        GameAudio::DungeonAmbientPath,
        0.24f
    );
    auto* flockingSystem = mainScene.AddComponent<FlockingSystem>();
    // Opcjonalne tunowanie:
    flockingSystem->separationRadius = 2.5f;
    flockingSystem->separationWeight = 1.8f;
    flockingSystem->alignmentRadius = 5.0f;
    flockingSystem->alignmentWeight = 0.3f;
    flockingSystem->cohesionRadius = 6.0f;
    flockingSystem->cohesionWeight = 0.2f;

// If Visual Studio doesn't like this I'm going to give up and force you guys to
// switch to GCC
#pragma region World

    ShaderProgram* skyProg =
        ShaderProgram::Build()
            .WithVertexShader(("./res/shaders/skybox.vert"))
            .WithPixelShader(("./res/shaders/skybox.frag"))
            .Link();

    Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
        "./res/textures/null_skybox.hdr", Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

    mainScene.GetRootNode()->AddObject<Skybox>(skyMat);

#pragma endregion
#pragma region Player
    JPH::Ref<JPH::CharacterVirtualSettings> characterSettings =
        new JPH::CharacterVirtualSettings();
    characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
    characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

    SceneNode* playerNode = mainScene.CreateNode("Player");
    playerNode->GlobalTransform().Position() = glm::vec3(3, 0, 0);

    SceneNode* bimberman =
        ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/bimbermann_throwing.glb")
            ->Instantiate(&mainScene, mainScene.root, "Bimberman");
    bimberman->SetParent(playerNode);

    auto* virtualCharacter =
        playerNode->AddObject<Physics::VirtualCharacterController>(
            characterSettings);
    virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);
    virtualCharacter->SetPosition(
        playerNode->GlobalTransform().Position().Value());
    virtualCharacter->SetGravityFactor(1);

    auto* aimingAid =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/crosshair.glb")
            ->Instantiate(&mainScene, playerNode, "Aim Reticle")
            ->AddObject<AimCrosshair>();

    auto* player = playerNode->AddObject<PlayerController>();

#pragma endregion
#pragma region Camera

    SceneNode* cameraNode =
        mainScene.LoadPrefab(fs::path("./res/prefabs/PlayerCamera.prefab"));

    CameraSettings* camera = cameraNode->GetObject<CameraSettings>();

    camera->SetHeight(7);
    camera->SetAngleY(225);

	auto* jfa = cameraNode->AddObjectIfMissing<JfaOutline>();
	jfa->outlineThickness = 4.0f;
	jfa->outlineColor = {1.0f, 1.0f, 1.0f};

#pragma endregion
#pragma region Miscellaneous
    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        {1.0f, 1.0f, 1.0f, 0.1f});
#pragma endregion
#pragma region Generator

    SceneNode* generatorNode = mainScene.CreateNode("Dungeon Generator");

    auto* dungeon = generatorNode->AddObject<DungeonGenerator>();

    dungeon->RemakeDungeon();

#pragma endregion
// #pragma region Ui
//
//     SceneNode* uiRoot = mainScene.CreateNode("UI");
//     SceneNode* pauseMenu = mainScene.CreateNode(uiRoot, "Pause Menu");
//     pauseMenu->AddObject<PauseMenu>();
//
// #pragma endregion

#pragma region Ui

    SceneNode* uiRoot = mainScene.CreateNode("UI");
    SceneNode* tabMenu = mainScene.CreateNode(uiRoot, "Tab Menu");
    tabMenu->AddObject<TabMenu>();
    SceneNode* pauseMenu = mainScene.CreateNode(uiRoot, "Pause Menu");
    pauseMenu->AddObject<PauseMenu>();
    SceneNode* inGameUi = mainScene.CreateNode(uiRoot, "HUD");
    inGameUi->AddObject<InGameUi>();

#pragma endregion

    return;
}
} // namespace DungeonGeneratorScene
