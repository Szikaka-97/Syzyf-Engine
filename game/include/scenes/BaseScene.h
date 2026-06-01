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
#include <text/Text3D.h>

#include <Application.h>
#include <algorithm>
#include "TutorialThrowingRoomScene.h"

namespace BaseScene {

class GateKey : public PickableItem {
	virtual void OnPickUp() {
		PersistentData::Set<bool>("Base_PlayerPickedUpKey", true);
	}
};

inline int GetBasePointLightIndex(const std::string& name) {
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

inline void CollectBasePointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
	if (!node) {
		return;
	}

	std::string nodeName = node->GetName();

	if (nodeName.rfind("PointLight", 0) == 0) {
		pointLights.push_back(node);
	}

	for (SceneNode* child : node->GetChildren()) {
		CollectBasePointLightsRecursive(child, pointLights);
	}
}

class BaseLights : public GameObject {
private:
	std::vector<Light*> lights;
	std::vector<float> baseIntensities;

public:
	void Awake() {
		std::vector<SceneNode*> pointLightNodes;

		CollectBasePointLightsRecursive(GetNode(), pointLightNodes);

		std::sort(pointLightNodes.begin(), pointLightNodes.end(), [](SceneNode* a, SceneNode* b) {
			return GetBasePointLightIndex(a->GetName()) < GetBasePointLightIndex(b->GetName());
		});

		for (SceneNode* pointLightNode : pointLightNodes) {
			Light* light = nullptr;

			if (!pointLightNode->TryGetObject<Light>(light)) {
				light = pointLightNode->AddObject<Light>(
					Light::PointLight(
						glm::vec3(1.0f, 0.5f, 0.1f),
						10.0f,
						0.0f,
						0.09f,
						0.032f
					)
				);
			}

			this->lights.push_back(light);
			this->baseIntensities.push_back(3.0f);
		}

		spdlog::info("BaseLights: prepared {} point lights.", this->lights.size());
	}

	void Update() {
		float lightsOnTime = PersistentData::Get<float>("Base_TurnLightsOn");

		for (int index = 0; index < this->lights.size(); index++) {
			Light* light = this->lights[index];
			float flicker = glm::sin(Time::Current() * (0.6f + (light->GetID() % 4) * 0.15f) + light->GetID() * 4.0f) * 0.5f;
			float targetIntensity = this->baseIntensities[index] + flicker;
			float turnOnAmount = 1.0f;

			if (index >= 2) {
				if (lightsOnTime == 0.0f) {
					turnOnAmount = 0.0f;
				}
				else {
					float sequenceDelay = float(index - 2) * 0.25f;
					float fadeDuration = 0.6f;
					turnOnAmount = glm::clamp((Time::Current() - lightsOnTime - sequenceDelay) / fadeDuration, 0.0f, 1.0f);
				}
			}

			light->SetIntensity(targetIntensity * turnOnAmount);
		}
	}
};

class BaseScript : public GameObject { // Move to own file later
private:
	SceneNode* gate;
	glm::vec3 exitVolume;
	SceneNode* key;
	bool gateLowering = false;

public:
	void Awake() {
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
		gateColliderNode->GlobalTransform().Position() = this->gate->GlobalTransform().Position().Value();

		assert(gateColliderNode);

		auto gateBodySettings = JPH::BodyCreationSettings{
			Physics::MeshShape(gateColliderNode->GetObject<MeshRenderer>()->GetMesh()),
			JPH::RVec3(0, 0, 0), JPH::Quat::sZero(), JPH::EMotionType::Static,
			Physics::Layers::NON_MOVING
		};
		gateBodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		gateBodySettings.mMassPropertiesOverride.mMass = 1.0;

		auto* gateBody = gateColliderNode->AddObject<Physics::Body>(gateBodySettings);
		gateBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		gateBody->SetPosition(gateColliderNode->GlobalTransform().Position());
		gateColliderNode->GetObject<MeshRenderer>()->SetEnabled(false);
	}

	void Update() {
		if (!this->gateLowering && glm::distance(
			PlayerController::Instance()->GlobalTransform().Position().Value(),
			this->gate->GlobalTransform().Position().Value()
		) < 2) {
			if (PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
				this->gateLowering = true;
			}
			else {
				PersistentData::Set<bool>("Base_PlayerBoopedGate", true);
			}
		}

		if (this->gateLowering) {
			glm::vec3 gatePos = this->gate->GlobalTransform().Position();

			if (gatePos.y > -3) {
				gatePos.y -= Time::Delta() * 2;
			}

			this->gate->GlobalTransform().Position() = gatePos;

			this->gate->GetAllObjectsInChildren<Physics::Body>()[0]->SetPosition(this->gate->GlobalTransform().Position());
		}
	}
};

class BaseTutorialManager : public GameObject {
private:
	Text3D* tutorialText;
	float timePoint;
	bool playerStartedMoving = false;
	bool playerReachedDoors = false;
	bool playerMovedAwayFromDoors = false;
	bool playerFoundKey = false;
	bool playerGotCloseToKey = false;

	float TimeSincePoint() const {
		return Time::Current() - timePoint;
	}
public:
	void Awake() {
		TextureParams fontTextureParams = {
			.channels = TextureChannels::RGB,
			.colorSpace = TextureColor::Linear,
			.format = TextureFormat::Ubyte,
			.wrapU = TextureWrap::Clamp,
			.wrapV = TextureWrap::Clamp,
			.minFilter = TextureFilter::Linear,
			.magFilter = TextureFilter::Linear
		};

		Texture2D* papyrusAtlas = GetScene()->Resources()->Get<Texture2D>(
			"./res/fonts/Papyrus/Papyrus-Regular.png",
			fontTextureParams
		);
		Font* papyrusFont = GetScene()->Resources()->Get<Font>(
			"./res/fonts/Papyrus/Papyrus-Regular.json",
			papyrusAtlas,
			true
		);

		SceneNode* text3dNode = GetScene()->CreateNode("Text 3D");
		text3dNode->LocalTransform().Position() = {3.0f, 2.0f, 1.0f};
		text3dNode->GlobalTransform().Scale() = glm::vec3(0.4f);
		this->tutorialText = text3dNode->AddObject<Text3D>(" ", papyrusFont);
		this->tutorialText->color = {1.2f, 0.3f, 0.0f, 0.0f};
		this->tutorialText->billboardMode = BillboardMode::Enabled;
		this->tutorialText->SetAlignment(TextAlignment::Middle);

		this->timePoint = Time::Current();
	}

	void Update() {
		if (!playerStartedMoving) {
			if (GetScene()->Input()->KeyPressed(Key::W) | GetScene()->Input()->KeyPressed(Key::A) | GetScene()->Input()->KeyPressed(Key::S) | GetScene()->Input()->KeyPressed(Key::D)) {
				playerStartedMoving = true;

				timePoint = Time::Current();
			}

			if (TimeSincePoint() > 3) {
				this->tutorialText->SetText("You can use WASD to move around");
				this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w + Time::Delta(), 0.f, 1.f);
			}
			return;
		}
		else if (!PersistentData::Get<bool>("Base_PlayerBoopedGate")) {
			this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w - Time::Delta() * 2, 0.f, 1.f);
		}

		if (PersistentData::Get<bool>("Base_PlayerBoopedGate") && !this->playerMovedAwayFromDoors) {
			if (!this->playerReachedDoors) {
				this->playerReachedDoors = true;

				this->timePoint = Time::Current();
			}

			this->tutorialText->GlobalTransform().Position() = glm::vec3(1.6, 2, 8.5);

			if (TimeSincePoint() > 0.8 && PersistentData::Get<float>("Base_TurnLightsOn") == 0) {
				spdlog::info("Turning on the lights");
				PersistentData::Set<float>("Base_TurnLightsOn", Time::Current());
			}
			if (TimeSincePoint() > 0.5) {
				this->tutorialText->SetText("The gate is locked\nFind a key to open it");

				float distFromDoors = glm::distance(
					PlayerController::Instance()->GlobalTransform().Position().Value(),
					GetNode()->FindNode("Exit Gate")->GlobalTransform().Position().Value()
				);

				this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w + Time::Delta(), 0.f, glm::min(1.0f, 1.0f + 1.f - distFromDoors / 2));
				
				if (distFromDoors > 4) {
					this->tutorialText->color.w = 0;

					this->timePoint = Time::Current();

					this->playerMovedAwayFromDoors = true;
				}
			}
		}

		if (this->playerMovedAwayFromDoors && !this->playerGotCloseToKey) {
			if (TimeSincePoint() > 1) {
				this->tutorialText->GlobalTransform().Position() = PlayerController::Instance()->GlobalTransform().Position() + glm::vec3(0, 1.5, 0);
				this->tutorialText->SetText("Use Q + E to rotate the camera");
				this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w + Time::Delta(), 0.f, 1.f);

				if (!PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
					GetNode()->GetObjectInChildren<GateKey>()->GetObject<MeshRenderer>()->maskFlags = MaskEffectBits::Outline;
				}
			}

			CameraSettings* cam = GetScene()->FindObjectsOfType<CameraSettings>()[0];

			if (cam->angleY > 180) {
				this->playerFoundKey = true;
			}
		}

		if (this->playerFoundKey && !this->playerGotCloseToKey) {
			this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w - Time::Delta() * 2, 0.f, 1.f);

			if (!PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
				float distToKey = glm::distance(
					GetNode()->GetObjectInChildren<GateKey>()->GlobalTransform().Position().Value(),
					PlayerController::Instance()->GlobalTransform().Position().Value()
				);
				
				if (distToKey < 1) {
					this->playerGotCloseToKey = true;

					this->timePoint = Time::Current();
				}
			}
		}

		if (this->playerGotCloseToKey) {
			this->tutorialText->SetText("Use G to pick up objects that are near you");

			if (!PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
				this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w + Time::Delta(), 0.f, 1.f);
			}
			else {
				this->tutorialText->color.w = glm::clamp(this->tutorialText->color.w - Time::Delta(), 0.f, 1.f);
			}
		}
	}
};

class BaseExitToTutorialThrowingRoom : public GameObject {
private:
	bool sceneRequested = false;
	glm::vec3 triggerPosition = glm::vec3(1.6686f, 0.0f, 20.0f);
	float triggerRadius = 2.5f;

public:
	void Awake() {
		SceneNode* triggerNode = GetNode()->FindNode("NextRoom");

		if (triggerNode) {
			this->triggerPosition = triggerNode->GlobalTransform().Position().Value();
		}
	}

	void Update() {
		if (this->sceneRequested) {
			return;
		}

		if (!PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
		 	return;
                }

                float distanceToTrigger = glm::distance(
                        PlayerController::Instance()->GlobalTransform().Position().Value(),
                        this->triggerPosition
                );

                if (distanceToTrigger < this->triggerRadius) {
                        this->sceneRequested = true;

                        Application::Get()->RequestSceneBuild(
                        [](Scene* s) { TutorialThrowingRoomScene::InitScene(*s); }
                        );
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

	mainScene.GetComponent<LightSystem>()->SetAmbientLight(glm::vec4(1, 0.6, 0.3, 0.03));

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

	floorNode->AddObject<Skybox>(skyMat);

	auto* exitFogNode = mainScene.CreateNode(floorNode, "Exit Fog");
	exitFogNode->GlobalTransform().Position() = glm::vec3(1.6686f, -1, 20);
	exitFogNode->GlobalTransform().Rotation() = glm::radians(glm::vec3(-8, 0, 0));
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
	JPH::Ref<JPH::CharacterVirtualSettings> characterSettings = new JPH::CharacterVirtualSettings();
	characterSettings->mShape = new JPH::CapsuleShape(0.5f, 0.5f);
	characterSettings->mShapeOffset = JPH::Vec3(0, 1, 0);
	characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);

	SceneNode* playerNode = mainScene.CreateNode("Player");

	SceneNode* bimberman = ResourceDatabase::Global->Get<GltfScene>(
		"./res/models/bimbermann_throwing.glb"
	)->Instantiate(&mainScene, mainScene.root, "Bimberman");

	bimberman->SetParent(playerNode);

	playerNode->GlobalTransform().Position() = glm::vec3(4, 0, -1);

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
