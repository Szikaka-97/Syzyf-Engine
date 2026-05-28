#pragma once

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <physics/System.h>
#include <animation/AnimationSystem.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>

#include <GltfScene.h>
#include <Graphics.h>
#include <Skybox.h>
#include <LightSystem.h>
#include <MeshRenderer.h>
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

#include <ui/widgets/UiCircularBar.h>
#include <ui/widgets/wheel/UiRadialWheel.h>
#include <ui/objects/UiCursor.h>
#include <ui/objects/UiInteractable.h>
#include <ui/objects/UiLayout.h>
#include <ui/objects/UiScrollableGrid.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiVisual.h>
#include <ui/systems/UiSystem.h>

#include <fog/FogVolume.h>
#include <PersistentData.h>

namespace BaseScene {

class GateKey : public PickableItem {
	virtual void OnPickUp() {
		PersistentData::Set<bool>("Base_PlayerPickedUpKey", true);
	}
};

class BaseScript : public GameObject { // Move to own file later
private:
	SceneNode* gate;
	glm::vec3 exitVolume;
	SceneNode* key;

	bool gateLowering = false;
public:
	BaseScript() {
		this->gate = GetNode()->FindNode("Exit Gate");
		this->key = GetNode()->FindNode("Gate Key");

		assert(this->gate != nullptr);
		assert(this->key != nullptr);

		this->key->AddObject<GateKey>();

		if (PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
			this->key->SetEnabled(false);
		}

		auto* wallsColliderNode = GetNode()->FindNode("Walls Collider");

		assert(wallsColliderNode);

		auto* wallsBody = wallsColliderNode->AddObject<Physics::Body>(
			JPH::BodyCreationSettings{
				Physics::MeshShape(wallsColliderNode->GetObject<MeshRenderer>()->GetMesh()),
				JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
				Physics::Layers::NON_MOVING
			}
		);
		wallsBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		wallsColliderNode->GetObject<MeshRenderer>()->SetEnabled(false);

		auto* gateColliderNode = this->gate->FindNode("Exit Gate Collider");

		assert(gateColliderNode);

		auto gateBodySettings = JPH::BodyCreationSettings{
			Physics::MeshShape(gateColliderNode->GetObject<MeshRenderer>()->GetMesh()),
			JPH::RVec3(1, 2, 3), JPH::Quat::sZero(), JPH::EMotionType::Kinematic,
			Physics::Layers::NON_MOVING
		};
		gateBodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		gateBodySettings.mMassPropertiesOverride = JPH::MassProperties(0, JPH::Mat44::sZero());

		auto* gateBody = gateColliderNode->AddObject<Physics::Body>(gateBodySettings);
		gateBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		gateBody->SetPosition(gateColliderNode->GlobalTransform().Position());
		// gateColliderNode->GetObject<MeshRenderer>()->SetEnabled(false);
	}

	void Update() {
		if (!this->gateLowering && glm::distance(
			PlayerController::Instance()->GlobalTransform().Position().Value(),
			this->gate->GlobalTransform().Position().Value()
		) < 1 && PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
			this->gateLowering = true;
		}

		if (this->gateLowering) {
			glm::vec3 gatePos = this->gate->GlobalTransform().Position();

			if (gatePos.y > -3) {
				gatePos.y -= Time::Delta() * 2;
			}

			this->gate->GlobalTransform().Position() = gatePos;
		}
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

#pragma region Base
	auto floorNode = ResourceDatabase::Global->Get<GltfScene>("./res/models/rooms/Base.glb")->Instantiate(&mainScene, mainScene.root, "Floor");
	MeshRenderer* floorMeshRenderer = floorNode->GetObjectInChildren<MeshRenderer>();
	auto* floorBody = floorMeshRenderer->GetNode()->AddObject<Physics::Body>(
		JPH::BodyCreationSettings{
			Physics::MeshShape(floorMeshRenderer->GetMesh()),
			JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
			Physics::Layers::NON_MOVING
		}
	);
	floorBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
	mainScene.GetComponent<LightSystem>()->SetAmbientLight({1.0f, 1.0f, 1.0f, 0.6f});

	ShaderProgram* skyProg = ShaderProgram::Build()
	.WithVertexShader(("./res/shaders/skybox.vert"))
	.WithPixelShader(("./res/shaders/skybox.frag"))
	.Link();

	Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
		"./res/textures/citrus_orchard_road_puresky.hdr",
		Texture::HDRColorBuffer
	);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	floorNode->AddObject<Skybox>(skyMat);

	auto* exitFogNode = mainScene.CreateNode(floorNode, "Exit Fog");
	exitFogNode->GlobalTransform().Position() = glm::vec3(1.6686f, 0, 12.25);
	exitFogNode->GlobalTransform().Scale() = glm::vec3(4, 4, 4);
	auto* exitFog = exitFogNode->AddObject<FogVolume>();
	exitFog->scatteringDensity = 0.5;
	exitFog->absorptionDensity = 0.1;
	exitFog->coverage = 0.1;
	exitFog->sharpness = 3.5;

	floorNode->AddObject<BaseScript>();
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

	auto* virtualCharacter = playerNode->AddObject<Physics::VirtualCharacterController>(characterSettings);
	virtualCharacter->SetCollisionLayerAndMask({1}, 0xFFFFFFFF);
	virtualCharacter->SetPosition(playerNode->GlobalTransform().Position().Value());
	virtualCharacter->SetGravityFactor(1);
	
	auto* player = playerNode->AddObject<PlayerController>();
#pragma endregion

#pragma region Camera
	SceneNode* cameraNode = mainScene.CreateNode("Camera Node");
	cameraNode->AddObject<Camera>(Camera::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 200.0f));
	cameraNode->AddObject<CameraSettings>(playerNode->GlobalTransform().Position())->angleY = 135;
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

#pragma region Gameplay
#pragma endregion

#pragma region UI
// SceneNode* uiRoot = mainScene.CreateNode("UI");

// 	// Move this into the wheel system
// 	SceneNode* uiNode = mainScene.CreateNode(uiRoot, "Ui Node");
// 	uiNode->AddObject<WheelTag>();
// 	uiNode->AddObject<UiLayout>(glm::uvec2(400, 400), glm::uvec2(150, 0), 0,
// 								AnchorPoint::CenterLeft);
// 	auto* uiVisual = uiNode->AddObject<UiVisual>(
// 		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
// 		mainScene.Resources()->Get<Texture2D>(
// 			"./res/textures/1147437805040054272.png",
// 			Texture2D::ColorTextureRGBA));
// 	uiVisual->SetEnabled(false);
// 	uiVisual->colorHovered = {1.0f, 0.0f, 0.0f, 1.0f};
// 	uiVisual->colorClicked = {0.0f, 1.0f, 0.0f, 1.0f};
// 	uiNode->AddObject<UiInteractable>();

// 	SceneNode* cursorNode = mainScene.CreateNode(uiRoot, "Cursor");
// 	cursorNode->AddObject<UiLayout>(glm::uvec2(64, 64), glm::uvec2(0, 0), 9999);

// 	cursorNode->AddObject<UiVisual>(
// 		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
// 		mainScene.Resources()->Get<Texture2D>(
// 			"./res/textures/1147437805040054272.png",
// 			Texture2D::ColorTextureRGBA));
// 	cursorNode->AddObject<UiCursor>();

// 	ShaderProgram* customUiProgram =
// 		ShaderProgram::Build()
// 			.WithVertexShader("./res/shaders/ui/ui.vert")
// 			.WithPixelShader("./res/shaders/ui/custom/radial_wheel.frag")
// 			.Link();
// 	Material* customUiMaterial = new Material(customUiProgram);
// 	SceneNode* radialWheelNode = mainScene.CreateNode(uiRoot, "Radial Wheel");
// 	radialWheelNode->AddObject<UiLayout>(
// 		glm::uvec2(600, 600), glm::uvec2(-150, 0), 0, AnchorPoint::CenterRight);
// 	auto* customVisual =
// 		radialWheelNode->AddObject<UiVisual>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
// 	customVisual->SetEnabled(false);
// 	customVisual->customMaterial = customUiMaterial;
// 	auto* radialWheel = radialWheelNode->AddObject<UiRadialWheel>();
// 	radialWheel->AddObject<WheelTag>();
// 	radialWheel->material.reset(customUiMaterial);
// 	radialWheel->SetItemModels({
// 		"./res/models/butelka.glb",
// 		"./res/models/butelka.glb",
// 		"./res/models/butelka.glb",
// 		"./res/models/butelka.glb",
// 		"./res/models/butelka.glb",
// 	});

// 	SceneNode* gridRoot = mainScene.CreateNode(uiRoot, "Grid");
// 	gridRoot->AddObject<UiLayout>(glm::uvec2(360, 475), glm::uvec2(50, 50), 0,
// 								  AnchorPoint::TopLeft);
// 	auto* gridRootVisual =
// 		gridRoot->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
// 	gridRootVisual->SetEnabled(false);
// 	gridRoot->AddObject<WheelTag>();
// 	SceneNode* gridContainer = mainScene.CreateNode(gridRoot, "Grid Container");
// 	auto* gridLayout = gridContainer->AddObject<UiLayout>(
// 		glm::uvec2(330, 445), glm::uvec2(0, 0), 0, AnchorPoint::Center);
// 	// gridContainer->AddObject<UiVisual>(glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
// 	gridContainer->AddObject<UiInteractable>();

// 	auto* grid = gridContainer->AddObject<UiScrollableGrid>();
// 	for (int i = 0; i < 20; i++) {
// 		SceneNode* itemNode =
// 			mainScene.CreateNode(uiRoot, "Item_" + std::to_string(i));
// 		itemNode->SetParent(gridContainer);

// 		auto* layout = itemNode->AddObject<UiLayout>(
// 			glm::uvec2(100, 100), glm::uvec2(0, 0), 1, AnchorPoint::Center);

// 		auto* visual =
// 			itemNode->AddObject<UiVisual>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
// 		visual->SetEnabled(false);
// 		itemNode->AddObject<UiInteractable>();
// 		itemNode->AddObject<WheelTag>();
// 	}
#pragma endregion
}

};
