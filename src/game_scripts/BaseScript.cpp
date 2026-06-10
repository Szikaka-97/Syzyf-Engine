#include <game_scripts/BaseScript.h>

#include <MeshRenderer.h>
#include <PersistentData.h>
#include <Light.h>
#include <InputSystem.h>
#include <Application.h>
#include <physics/Body.h>
#include <physics/System.h>
#include <physics/Helpers.h>
#include <text/Text3D.h>
#include <text/Font.h>
#include <game_scripts/CameraSettings.h>
#include <game_scripts/PlayerController.h>
#include "../../game/include/scenes/TutorialThrowingRoomScene.h"

int GetBasePointLightIndex(const std::string& name) {
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

void CollectBasePointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
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

void GateKey::OnPickUp() {
	PersistentData::Set<bool>("Base_PlayerPickedUpKey", true);
}

void BaseLights::Awake() {
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
}

void BaseLights::Update() {
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

void BaseScript::Awake() {
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

void BaseScript::Update() {
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

void BaseTutorialManager::Awake() {
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

void BaseTutorialManager::Update() {
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

		if (cam->GetAngleY() > 180) {
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

	if (PersistentData::Get<bool>("Base_PlayerPickedUpKey")) {
		if (!GetObject<BaseScript>()->gateLowering) {
			GetNode()->FindNode("Exit Gate/Exit Gate Lock")->GetObject<MeshRenderer>()->maskFlags = MaskEffectBits::Outline;
		}
		else {
			GetNode()->FindNode("Exit Gate/Exit Gate Lock")->GetObject<MeshRenderer>()->maskFlags = 0;
		}
	}
}

void BaseExitToTutorialThrowingRoom::Awake() {
	SceneNode* triggerNode = GetNode()->FindNode("NextRoom");

	if (triggerNode) {
		this->triggerPosition = triggerNode->GlobalTransform().Position().Value();
	}
}

void BaseExitToTutorialThrowingRoom::Update() {
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

			spdlog::error("BUILD");

			Application::Get()->RequestSceneBuild(
			[](Scene* s) { TutorialThrowingRoomScene::InitScene(*s); }
			);
	}
}
