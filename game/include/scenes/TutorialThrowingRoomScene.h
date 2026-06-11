#pragma once

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <physics/System.h>
#include <animation/AnimationSystem.h>
#include <animation/AnimationComponent.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiText.h>
#include <game_scripts/PickableItem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObject.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/AimCrosshair.h>
#include <game_scripts/enemies/EnemyBase.h>

#include <GltfScene.h>
#include <Graphics.h>
#include <Skybox.h>
#include <LightSystem.h>
#include <MeshRenderer.h>
#include <Surface.h>
#include <physics/Body.h>
#include <physics/Helpers.h>
#include <physics/VirtualCharacterController.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/CameraSettings.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Camera.h>
#include <MaskEffects.h>
#include <JfaOutline.h>
#include <Bloom.h>
#include <Tonemapper.h>
#include <ColorGrading.h>
#include <Fxaa.h>
#include "CraftingScene.h"
#include <PersistentData.h>
#include <Application.h>
#include <algorithm>
#include <limits>
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

	mainScene.GetComponent<LightSystem>()->SetAmbientLight(glm::vec4(1, 0.6, 0.3, 0.035));

    auto* flockingSystem = mainScene.AddComponent<FlockingSystem>();
    flockingSystem->separationRadius = 2.5f;
    flockingSystem->separationWeight = 1.8f;
    flockingSystem->alignmentRadius = 5.0f;
    flockingSystem->alignmentWeight = 0.3f;
    flockingSystem->cohesionRadius = 6.0f;
    flockingSystem->cohesionWeight = 0.2f;
    spdlog::error("init");

#pragma region Room
	SceneNode* roomNode = ResourceDatabase::Global->Get<GltfScene>(
		"./res/models/rooms/TutorialThrowingRoom.glb"
	)->Instantiate(&mainScene, mainScene.root, "Tutorial Throwing Room");

	Surface* surface = AddRoomPhysicsAndSurface(roomNode);

	ShaderProgram* skyProg = ShaderProgram::Build()
	.WithVertexShader(("./res/shaders/skybox.vert"))
	.WithPixelShader(("./res/shaders/skybox.frag"))
	.Link();

	Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
		"./res/textures/null_skybox.hdr",
		Texture::HDRColorBuffer
	);
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

#pragma region Player
	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings = new JPH::CharacterVirtualSettings();
	characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
	characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	SceneNode* playerNode = mainScene.CreateNode("Player");

	SceneNode* bimberman = ResourceDatabase::Global->Get<GltfScene>(
		"./res/models/bimbermann_throwing.glb"
	)->Instantiate(&mainScene, mainScene.root, "Bimberman");

	bimberman->SetParent(playerNode);

	SceneNode* playerSpawn = roomNode->FindNode("Player Spawn");

	if (playerSpawn) {
		playerNode->GlobalTransform().Position() = playerSpawn->GlobalTransform().Position().Value();
	}
	else {
		playerNode->GlobalTransform().Position() = glm::vec3(14.183422f, 0.105301f, -0.051525f);
		spdlog::warn("TutorialThrowingRoomScene: Player Spawn not found. Using fallback from uploaded GLB.");
	}

	auto* virtualCharacter = playerNode->AddObject<Physics::VirtualCharacterController>(characterSettings);
	virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);
	virtualCharacter->SetPosition(playerNode->GlobalTransform().Position().Value());
	virtualCharacter->SetGravityFactor(1);

	SceneNode* aimReticle = ResourceDatabase::Global->Get<GltfScene>("./res/models/crosshair.glb")
		->Instantiate(&mainScene, playerNode, "Aim Reticle");

	aimReticle->AddObject<AimCrosshair>();
	aimReticle->SetEnabled(PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles"));

	auto* player = playerNode->AddObject<PlayerController>();

	player->SetThrowingUnlocked(
		PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")
	);

	roomNode->AddObject<TutorialRatSpawnManager>()->Initialize(
		roomNode,
		playerNode,
		surface
	);

	roomNode->AddObject<TutorialThrowingRoomDoorOpener>()->Initialize(
		playerNode,
		FindTutorialDoorNode(roomNode)
	);

	roomNode->AddObject<TutorialThrowingRoomExitToCrafting>();
#pragma endregion

#pragma region Camera
	SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
	cameraNode->AddObject<Camera>(Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	cameraNode->AddObject<CameraSettings>(playerNode->GlobalTransform().Position(), 7, 225);
	cameraNode->AddObject<MaskEffects>();

	auto* jfa = cameraNode->AddObject<JfaOutline>();
	jfa->outlineThickness = 4.0f;
	jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};

	auto* dof = cameraNode->AddObject<DepthOfField>();
	dof->SetEnabled(false);

	cameraNode->AddObject<MaskEffects>();
	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);
	cameraNode->AddObject<ColorGrading>();
	cameraNode->AddObject<Fxaa>();
#pragma endregion
}

}