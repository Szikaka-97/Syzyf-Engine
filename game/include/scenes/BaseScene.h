#pragma once

#include "Noise3D.h"
#include "Resources.h"
#include "fog/FluidSimulation.h"

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <animation/AnimationSystem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/ui/InGameUi.h>
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

class CameraSettings;

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
    auto* fluidSim = exitFogNode->AddObject<FluidSimulation>();
    auto* fogVolume = exitFogNode->AddObject<FogVolume>();

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
    fogVolume->scatteringColor = {0.130f, 0.115f, 0.101f};
    fogVolume->emissiveStrength = 0.784;
    fogVolume->noiseScale = 0.090f;
    fogVolume->velocityTexture = fluidSim->GetVelocityMap();
    fogVolume->velocityStrength = 5.0f;

    fluidSim->damping = 0.948f;
    fluidSim->playerRadius = 0.033f;
    fluidSim->interactionStrength = 0.840f;

    floorNode->AddObject<BaseScript>();
    floorNode->AddObject<BaseLights>();
    floorNode->AddObject<BaseTutorialManager>();
    floorNode->AddObject<BaseExitToTutorialThrowingRoom>();
#pragma endregion

#pragma region Player
	SceneNode* playerNode = mainScene.LoadPrefab(fs::path("./res/prefabs/Player.prefab"));
#pragma endregion

#pragma region Camera
    SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
    cameraNode->AddObject<Camera>(
        Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
    cameraNode->AddObject<CameraSettings>(
        playerNode->GlobalTransform().Position(), 5, 135);
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
    cameraNode->AddObject<MaskEffects>();
    cameraNode->AddObject<Fxaa>();
#pragma endregion

#pragma region Gameplay
#pragma endregion

#pragma region Ui
    // Wheel
    SceneNode* uiRoot = mainScene.CreateNode("UI");
    uiRoot->AddObject<InGameUi>();

#pragma endregion
}

}; // namespace BaseScene
