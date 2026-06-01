#pragma once

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <physics/System.h>
#include <animation/AnimationSystem.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>
#include <game_scripts/PickableItem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/AimCrosshair.h>

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

#include <PersistentData.h>
#include <algorithm>

namespace TutorialThrowingRoomScene {

class TutorialBottlePickup : public PickableItem {
public:
	virtual void OnPickUp() override {
		PersistentData::Set<bool>("TutorialThrowingRoom_PlayerTookBottles", true);

		if (PlayerController::Instance()) {
			PlayerController::Instance()->SetThrowingUnlocked(true);
		}
	}
};

inline MeshRenderer* FindMeshRenderer(SceneNode* node) {
	if (!node) {
		return nullptr;
	}

	return node->GetObjectInChildren<MeshRenderer>();
}

inline Surface* AddRoomPhysicsAndSurface(SceneNode* roomNode) {
	SceneNode* floorNode = roomNode->FindNode("FLOOR");
	SceneNode* wallsNode = roomNode->FindNode("WALLS");

	Surface* surface = nullptr;

	if (!floorNode) {
		spdlog::warn("TutorialThrowingRoomScene: FLOOR node not found.");
	}
	else {
		MeshRenderer* floorRenderer = FindMeshRenderer(floorNode);

		if (!floorRenderer) {
			spdlog::warn("TutorialThrowingRoomScene: FLOOR has no MeshRenderer.");
		}
		else {
			auto* floorBody = floorRenderer->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(floorRenderer->GetMesh()),
					JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING
				}
			);

			floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

			surface = floorRenderer->GetNode()->AddObject<Surface>(floorRenderer->GetMesh(), 1.0f);
			surface->SetID(roomNode->GetID());
		}
	}

	if (!wallsNode) {
		spdlog::warn("TutorialThrowingRoomScene: WALLS node not found.");
	}
	else {
		MeshRenderer* wallsRenderer = FindMeshRenderer(wallsNode);

		if (!wallsRenderer) {
			spdlog::warn("TutorialThrowingRoomScene: WALLS has no MeshRenderer.");
		}
		else {
			auto* wallsBody = wallsRenderer->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(wallsRenderer->GetMesh()),
					JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING
				}
			);

			wallsBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		}
	}

	return surface;
}

inline int GetPointLightIndex(const std::string& name) {
	const std::string prefix = "PointLight.";

	if (name.rfind(prefix, 0) != 0) {
		return 999999;
	}

	std::string numberText = name.substr(prefix.size());

	try {
		return std::stoi(numberText);
	}
	catch (...) {
		return 999999;
	}
}

inline void CollectPointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
	if (!node) {
		return;
	}

	std::string nodeName = node->GetName();

	if (nodeName.rfind("PointLight", 0) == 0) {
		pointLights.push_back(node);
	}

	for (SceneNode* child : node->GetChildren()) {
		CollectPointLightsRecursive(child, pointLights);
	}
}

inline void AddRoomLights(SceneNode* roomNode) {
	std::vector<SceneNode*> pointLights;

	CollectPointLightsRecursive(roomNode, pointLights);

	std::sort(pointLights.begin(), pointLights.end(), [](SceneNode* a, SceneNode* b) {
		return GetPointLightIndex(a->GetName()) < GetPointLightIndex(b->GetName());
	});

	int lightsAdded = 0;

	for (SceneNode* lightNode : pointLights) {
		Light* light = nullptr;

		if (!lightNode->TryGetObject<Light>(light)) {
			light = lightNode->AddObject<Light>(
				Light::PointLight(
					glm::vec3(1.0f, 0.55f, 0.18f),
					12.0f,
					4.0f,
					0.09f,
					0.032f
				)
			);

			lightsAdded++;
		}
		else {
			light->Set(
				Light::PointLight(
					glm::vec3(1.0f, 0.55f, 0.18f),
					12.0f,
					4.0f,
					0.09f,
					0.032f
				)
			);
		}
	}

	spdlog::info(
		"TutorialThrowingRoomScene: prepared {} point lights, added {} missing Light components.",
		pointLights.size(),
		lightsAdded
	);
}

inline void SetupBottlePickup(SceneNode* roomNode) {
	SceneNode* bottlesNode = roomNode->FindNode("Bottles");

	if (!bottlesNode) {
		spdlog::warn("TutorialThrowingRoomScene: Bottles node not found.");
		return;
	}

	if (PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
		bottlesNode->SetEnabled(false);
		return;
	}

	bottlesNode->AddObject<TutorialBottlePickup>();
}

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

#pragma region Room
	SceneNode* roomNode = ResourceDatabase::Global->Get<GltfScene>(
		"./res/models/rooms/TutorialThrowingRoom.glb"
	)->Instantiate(&mainScene, mainScene.root, "Tutorial Throwing Room");

	AddRoomPhysicsAndSurface(roomNode);

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

	AddRoomLights(roomNode);

	SetupBottlePickup(roomNode);
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

	ResourceDatabase::Global->Get<GltfScene>("./res/models/crosshair.glb")
		->Instantiate(&mainScene, playerNode, "Aim Reticle")
		->AddObject<AimCrosshair>();

	auto* player = playerNode->AddObject<PlayerController>();

	player->SetThrowingUnlocked(
		PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")
	);
#pragma endregion

#pragma region Camera
	SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
	cameraNode->AddObject<Camera>(Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	cameraNode->AddObject<CameraSettings>(playerNode->GlobalTransform().Position())->angleY = 180;
	cameraNode->AddObject<MaskEffects>();
	auto* jfa = cameraNode->AddObject<JfaOutline>();
	jfa->outlineThickness = 4.0f;
	jfa->outlineColor = {1.0f, 29.0f / 255.0f, 29.0f / 255.0f};
	auto* dof = cameraNode->AddObject<DepthOfField>();
	dof->SetEnabled(false);
	cameraNode->AddObject<Bloom>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);
	cameraNode->AddObject<ColorGrading>();
	cameraNode->AddObject<Fxaa>();
#pragma endregion
}

}