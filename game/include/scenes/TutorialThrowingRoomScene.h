#pragma once

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <animation/AnimationComponent.h>
#include <animation/AnimationSystem.h>
#include <game_scripts/AimCrosshair.h>
#include <game_scripts/PickableItem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObject.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/enemies/EnemyBase.h>
#include <game_scripts/ui/TabMenu.h>
#include <physics/System.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>

#include "CraftingScene.h"
#include "game_scripts/ui/InGame.h"
#include "game_scripts/ui/PauseMenu.h"
#include "ui/widgets/wheel/UiRadialWheel.h"
#include <Application.h>
#include <Bloom.h>
#include <Camera.h>
#include <ColorGrading.h>
#include <Fxaa.h>
#include <GltfScene.h>
#include <Graphics.h>
#include <JfaOutline.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <LightSystem.h>
#include <MaskEffects.h>
#include <MeshRenderer.h>
#include <PersistentData.h>
#include <Skybox.h>
#include <Surface.h>
#include <Tonemapper.h>
#include <algorithm>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <limits>
#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/VirtualCharacterController.h>
#include <string>
#include <vector>

#include <game_scripts/TutorialRoomScripts.h>

namespace TutorialThrowingRoomScene {

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<AnimationSystem>();
    mainScene.AddComponent<PickableItemSystem>();
    mainScene.AddComponent<TweenSystem>();
    mainScene.AddComponent<WheelSystem>();
    mainScene.AddComponent<ThrowableObjectPool>();

    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        glm::vec4(1, 0.6, 0.3, 0.035));

    // auto* flockingSystem = mainScene.AddComponent<FlockingSystem>();
    // flockingSystem->separationRadius = 2.5f;
    // flockingSystem->separationWeight = 1.8f;
    // flockingSystem->alignmentRadius = 5.0f;
    // flockingSystem->alignmentWeight = 0.3f;
    // flockingSystem->cohesionRadius = 6.0f;
    // flockingSystem->cohesionWeight = 0.2f;
    // spdlog::error("init");

#pragma region Room
    SceneNode* roomNode =
        ResourceDatabase::Global
            ->Get<GltfScene>("./res/models/rooms/TutorialThrowingRoom.glb")
            ->Instantiate(&mainScene, mainScene.root, "Tutorial Throwing Room");

    Surface* surface = AddRoomPhysicsAndSurface(roomNode);

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

    roomNode->AddObject<Skybox>(skyMat);
    roomNode->AddObject<TutorialThrowingRoomLights>();

    SetupBottlePickup(roomNode);

    roomNode->AddObject<TutorialThrowingPromptManager>();
#pragma endregion

#pragma region Torches

    // for (Light* light : mainScene.FindObjectsOfType<Light>()) {
    //     glm::vec3 pos = light->GetNode()->GlobalTransform().Position();
    //     SceneNode* torchNode = mainScene.CreateNode("torch");
    //     torchNode->GlobalTransform().Position() = pos;
    //     torchNode->AddObject<FireParticles>();
    // }

#pragma endregion

#pragma region Player
    SceneNode* playerNode =
        mainScene.LoadPrefab(fs::path("./res/prefabs/Player.prefab"));

    SceneNode* playerSpawn = roomNode->FindNode("Player Spawn");

    PlayerController* player = playerNode->GetObject<PlayerController>();

    if (playerSpawn) {
        player->SetPosition(playerSpawn->GlobalTransform().Position());
    } else {
        player->SetPosition(glm::vec3(14.183422f, 0.105301f, -0.051525f));

        spdlog::warn("TutorialThrowingRoomScene: Player Spawn not found. Using "
                     "fallback from uploaded GLB.");
    }

    // player->SetThrowingUnlocked(
    //     PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles"));

    roomNode->AddObject<TutorialRatSpawnManager>()->Initialize(
        roomNode, playerNode, surface);

    roomNode->AddObject<TutorialThrowingRoomDoorOpener>()->Initialize(
        playerNode, FindTutorialDoorNode(roomNode));

    roomNode->AddObject<TutorialThrowingRoomExitToCrafting>();
#pragma endregion

#pragma region Camera
    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position(), 7, 45);
    cameraNode->AddObject<MaskEffects>();

    auto* jfa = cameraNode->AddObject<JfaOutline>();
    jfa->outlineThickness = 4.0f;
    jfa->outlineColor = {1.0f, 1.0f, 1.0f};

    auto* dof = cameraNode->AddObject<DepthOfField>();
    dof->SetEnabled(false);

    cameraNode->AddObject<MaskEffects>();
    cameraNode->AddObject<Bloom>();
    cameraNode->AddObject<Tonemapper>()->SetOperator(
        Tonemapper::TonemapperOperator::GranTurismo);
    cameraNode->AddObject<ColorGrading>();
    cameraNode->AddObject<Fxaa>();
#pragma endregion

#pragma region Ui

    SceneNode* uiRoot = mainScene.CreateNode("UI");
    SceneNode* tabMenu = mainScene.CreateNode(uiRoot, "Tab Menu");
    tabMenu->AddObject<TabMenu>();
    SceneNode* pauseMenu = mainScene.CreateNode(uiRoot, "Pause Menu");
    pauseMenu->AddObject<PauseMenu>();
    SceneNode* inGameUi = mainScene.CreateNode(uiRoot, "HUD");
    inGameUi->AddObject<InGameUi>();

#pragma endregion
}

} // namespace TutorialThrowingRoomScene
