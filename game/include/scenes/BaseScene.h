#pragma once

#include "Resources.h"
#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <animation/AnimationSystem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/ui/InGame.h>
#include <game_scripts/ui/PauseMenu.h>
#include <game_scripts/ui/TabMenu.h>
#include <physics/System.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>

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
#include <Skybox.h>
#include <Tonemapper.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/VirtualCharacterController.h>

#include <ui/objects/UiCursor.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiScrollableGrid.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/UiCircularBar.h>
#include <ui/widgets/wheel/UiRadialWheel.h>

#include <PersistentData.h>
#include <fog/FogVolume.h>
#include <text/Text3D.h>

#include <Application.h>
#include <game_scripts/BaseScript.h>

namespace BaseScene {

class GateKey : public PickableItem {
    virtual void OnPickUp() {
        PersistentData::Set<bool>("Base_PlayerPickedUpKey", true);
    }
};

inline void InitScene(Scene& mainScene) {
    mainScene.AddComponent<Physics::System>();
    mainScene.AddComponent<DebugInspector>();
    mainScene.AddComponent<UiSystem>();
    mainScene.AddComponent<AnimationSystem>();
    mainScene.AddComponent<PickableItemSystem>();
    auto* tweenSystem = mainScene.AddComponent<TweenSystem>();
    mainScene.AddComponent<WheelSystem>();
    mainScene.AddComponent<ThrowableObjectPool>();

    mainScene.GetComponent<LightSystem>()->SetAmbientLight(
        glm::vec4(1, 0.6, 0.3, 0.13));

#pragma region Base
    auto floorNode =
        ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Base.glb")
            ->Instantiate(&mainScene, mainScene.root, "Floor");
    MeshRenderer* floorMeshRenderer =
        floorNode->FindNode("floor")->GetObject<MeshRenderer>();
    auto* floorBody = floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
        JPH::BodyCreationSettings{
            Physics::MeshShape(floorMeshRenderer->GetMesh()),
            JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
            Physics::Layers::NON_MOVING});
    floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

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

    floorNode->AddObject<Skybox>(skyMat);

    auto* exitFogNode = mainScene.CreateNode(floorNode, "Exit Fog");
    exitFogNode->GlobalTransform().Position() = glm::vec3(1.6686f, -1, 20);
    exitFogNode->GlobalTransform().Rotation() =
        glm::radians(glm::vec3(-8, 0, 0));
    exitFogNode->GlobalTransform().Scale() = glm::vec3(4, 4, 17);
    auto* exitFog = exitFogNode->AddObject<FogVolume>();
    exitFog->scatteringDensity = 0.5;
    exitFog->absorptionDensity = 0.1;
    exitFog->coverage = 0.1;
    exitFog->sharpness = 3.5;
    exitFog->emissiveStrength = 0.06;

    floorNode->AddObject<BaseScript>();
    floorNode->AddObject<BaseLights>();
    floorNode->AddObject<BaseTutorialManager>();
    floorNode->AddObject<BaseExitToTutorialThrowingRoom>();
#pragma endregion

#pragma region Player
    SceneNode* playerNode =
        mainScene.LoadPrefab(fs::path("./res/prefabs/Player.prefab"));
#pragma endregion

#pragma region Camera
    SceneNode* cameraNode =
        mainScene.LoadPrefab(fs::path("./res/prefabs/PlayerCamera.prefab"));

    CameraSettings* camera = cameraNode->GetObject<CameraSettings>();

    camera->SetHeight(5);
    camera->SetAngleY(135);
#pragma endregion

#pragma region Gameplay
#pragma endregion

#pragma region Ui
    // Wheel
    SceneNode* uiRoot = mainScene.CreateNode("UI");
    SceneNode* tabMenu = mainScene.CreateNode(uiRoot, "Tab Menu");
    tabMenu->AddObject<TabMenu>();
    SceneNode* pauseMenu = mainScene.CreateNode(pauseMenu, "Pause Menu");
    pauseMenu->AddObject<PauseMenu>();
    SceneNode* inGameUi = mainScene.CreateNode("HUD");
    inGameUi->AddObject<InGameUi>();

#pragma endregion
}

}; // namespace BaseScene
