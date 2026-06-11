#pragma once

#include "enemies/EnemySkeleton.h"


#include <Scene.h>
#include <PersistentData.h>
#include <TimeSystem.h>
#include <InputSystem.h>
#include <Light.h>
#include <Application.h>
#include <game_scripts/PickableItem.h>
#include <game_scripts/PlayerController.h>
#include <game_scripts/PotionInventory.h>
#include <game_scripts/enemies/EnemyBase.h>
#include <ui/objects/UiText.h>
#include <ui/objects/UiLayout.h>
#include <physics/Helpers.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

namespace CraftingScene {
inline void InitScene(Scene& mainScene);
}

static constexpr float RatSpawnExtraHeight = 0.0f;
static constexpr float RatCapsuleHalfHeight = 0.35f;
static constexpr float RatCapsuleRadius = 0.7f;
static constexpr float RatBodyCenterOffset = RatCapsuleHalfHeight + RatCapsuleRadius + RatSpawnExtraHeight;
static constexpr int MaxTutorialRatEnemies = 5;

inline void SpawnTutorialRats(Scene& mainScene, SceneNode* roomNode, SceneNode* playerNode, Surface* surface);

class TutorialBottlePickup : public PickableItem {
public:
	virtual void OnPickUp() override {
		PersistentData::Set<bool>("TutorialThrowingRoom_PlayerTookBottles", true);

		if (!PotionInventory::HasPotion()) {
			PotionInventory::SaveLastCraftedPotion(
			    "Basic Potion",
			    "Burn",
			    100.0f,
			    10,
			    false
			);

			PersistentData::Set<bool>(
			    PotionInventory::ShowTutorialFinishedMessageKey,
			    false
			);
		}

		if (PlayerController::Instance()) {
			PlayerController::Instance()->SetThrowingUnlocked(true);
		}
	}
};

class TutorialStaticRatTarget : public EnemySkeleton {
private:
	SceneNode* playerNode = nullptr;

	float damage = 10.0f;
	float damageRange = 1.6f;
	float damageCooldown = 1.0f;
	float damageTimer = 0.0f;

public:
	static int remainingRats;

	void Initialize(SceneNode* playerNode, float damage, float damageRange, float damageCooldown) {
		this->playerNode = playerNode;
		this->damage = damage;
		this->damageRange = damageRange;
		this->damageCooldown = damageCooldown;
		this->damageTimer = 0.0f;

		this->myNode = GetNode();
		this->currentPos = GetNode()->GlobalTransform().Position().Value();
		this->m_hp = 100;
	}

	void Awake() {
		this->myNode = GetNode();
		this->currentPos = GetNode()->GlobalTransform().Position().Value();

		remainingRats++;
	}

	void Update() {
		EnemySkeleton::Update();
		// this->myNode = GetNode();
		// this->currentPos = GetNode()->GlobalTransform().Position().Value();
		//
		// if (this->playerNode == nullptr) {
		// 	return;
		// }
		//
		// if (!PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
		// 	return;
		// }
		//
		// if (this->damageTimer > 0.0f) {
		// 	this->damageTimer -= Time::Delta();
		// 	return;
		// }
		//
		// float distanceToPlayer = glm::distance(
		// 	GetNode()->GlobalTransform().Position().Value(),
		// 	this->playerNode->GlobalTransform().Position().Value()
		// );
		//
		// if (distanceToPlayer <= this->damageRange) {
		// 	if (auto* player = this->playerNode->GetObject<PlayerController>()) {
		// 		player->TakeDamage(this->damage);
		// 		this->damageTimer = this->damageCooldown;
		// 	}
		// }
	}

	virtual void Die() override {
		remainingRats--;
		PotionInventory::GiveRatLoot();

		//spdlog::error("died3");
		EnemyBase::Die();

		spdlog::error("died4");
	}

	LootPool& GetLootPool() override {
		return LootPool::GetSkeletonLootPool();
	}
};

class TutorialRatSpawnManager : public GameObject {
private:
	SceneNode* roomNode = nullptr;
	SceneNode* playerNode = nullptr;
	Surface* surface = nullptr;
	bool ratsSpawned = false;

public:
	void Initialize(SceneNode* roomNode, SceneNode* playerNode, Surface* surface) {
		this->roomNode = roomNode;
		this->playerNode = playerNode;
		this->surface = surface;

		if (PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
			SpawnRatsIfNeeded();
		}
	}

	void SpawnRatsIfNeeded() {
		if (this->ratsSpawned || this->roomNode == nullptr || this->playerNode == nullptr) {
			return;
		}

		SpawnTutorialRats(*GetScene(), this->roomNode, this->playerNode, this->surface);

		this->ratsSpawned = true;
	}

	void Update() {
		if (!PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
			return;
		}

		SpawnRatsIfNeeded();
	}
};

class TutorialThrowingPromptManager : public GameObject {
private:
	UiText* promptText = nullptr;
	SceneNode* bottlesNode = nullptr;

	bool playerTookBottles = false;
	bool playerStartedAiming = false;
	bool playerReleasedThrow = false;

	float promptDistance = 3.0f;

	void HidePrompt() {
		this->promptText->color.w = glm::clamp(this->promptText->color.w - Time::Delta() * 4.0f, 0.0f, 1.0f);
	}

	void ShowPrompt(const std::string& text) {
		this->promptText->text = text;
		this->promptText->color.w = glm::clamp(this->promptText->color.w + Time::Delta() * 3.0f, 0.0f, 1.0f);
	}

	bool IsPlayerCloseToBottles() const {
		if (this->bottlesNode == nullptr || PlayerController::Instance() == nullptr) {
			return false;
		}

		float distanceToBottles = glm::distance(
			PlayerController::Instance()->GlobalTransform().Position().Value(),
			this->bottlesNode->GlobalTransform().Position().Value()
		);

		return distanceToBottles <= this->promptDistance;
	}

public:
	void Awake() {
		this->bottlesNode = GetNode()->FindNode("Bottles");
		this->playerTookBottles = PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles");

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

		SceneNode* uiTextNode = GetScene()->CreateNode("Throwing Tutorial Prompt UI");
		uiTextNode->AddObject<UiLayout>(
			glm::uvec2(460, 140),
			glm::ivec2(-40, 40),
			20,
			AnchorPoint::TopRight
		);

		this->promptText = uiTextNode->AddObject<UiText>("", papyrusFont);
		this->promptText->fontSize = 26.0f;
		this->promptText->alignment = TextAlignment::Right;
		this->promptText->maxWidth = 420.0f;
		this->promptText->color = {1.2f, 0.3f, 0.0f, 0.0f};
	}

	void Update() {
		if (this->promptText == nullptr) {
			return;
		}

		bool tookBottlesNow = PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles");

		if (tookBottlesNow && !this->playerTookBottles) {
			this->playerTookBottles = true;
		}

		if (!this->playerTookBottles) {
			if (IsPlayerCloseToBottles()) {
				ShowPrompt("Find the bottles\nPress G to pick them up");
			}
			else {
				HidePrompt();
			}

			return;
		}

		if (!this->playerStartedAiming) {
			ShowPrompt("Throw the potions\nat the rats");

			if (GetScene()->Input()->ButtonPressed(0)) {
				this->playerStartedAiming = true;
			}

			return;
		}

		if (!this->playerReleasedThrow) {
			if (GetScene()->Input()->ButtonPressed(0)) {
				ShowPrompt("Release LMB\nto throw");
			}
			else {
				this->playerReleasedThrow = true;
			}

			return;
		}

		if (TutorialStaticRatTarget::remainingRats > 0) {
			ShowPrompt("Hit the rats\nwith the potions");
		}
		else {
			ShowPrompt("Move to the next room");
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

	if (floorNode) {
		MeshRenderer* floorRenderer = FindMeshRenderer(floorNode);

		if (floorRenderer) {
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
		else {
			spdlog::warn("TutorialThrowingRoomScene: FLOOR has no MeshRenderer.");
		}
	}
	else {
		spdlog::warn("TutorialThrowingRoomScene: FLOOR node not found.");
	}

	if (wallsNode) {
		MeshRenderer* wallsRenderer = FindMeshRenderer(wallsNode);

		if (wallsRenderer) {
			auto* wallsBody = wallsRenderer->GetNode()->AddObject<Physics::Body>(
				JPH::BodyCreationSettings{
					Physics::MeshShape(wallsRenderer->GetMesh()),
					JPH::RVec3::sZero(), JPH::Quat::sZero(), JPH::EMotionType::Static,
					Physics::Layers::NON_MOVING
				}
			);

			wallsBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);
		}
		else {
			spdlog::warn("TutorialThrowingRoomScene: WALLS has no MeshRenderer.");
		}
	}
	else {
		spdlog::warn("TutorialThrowingRoomScene: WALLS node not found.");
	}

	return surface;
}

inline int GetPointLightIndex(const std::string& name) {
	const std::string prefix = "PointLight.";

	if (name.rfind(prefix, 0) != 0) {
		return 999999;
	}

	try {
		return std::stoi(name.substr(prefix.size()));
	}
	catch (...) {
		return 999999;
	}
}

inline void CollectPointLightsRecursive(SceneNode* node, std::vector<SceneNode*>& pointLights) {
	if (!node) {
		return;
	}

	if (node->GetName().rfind("PointLight", 0) == 0) {
		pointLights.push_back(node);
	}

	for (SceneNode* child : node->GetChildren()) {
		CollectPointLightsRecursive(child, pointLights);
	}
}

class TutorialThrowingRoomLights : public GameObject {
private:
	std::vector<Light*> lights;
	std::vector<float> baseIntensities;

	float lightsOnTime = 0.0f;
	float sequenceDelay = 0.55f;
	float fadeDuration = 1.25f;

public:
	void Awake() {
		std::vector<SceneNode*> pointLightNodes;

		CollectPointLightsRecursive(GetNode(), pointLightNodes);

		std::sort(pointLightNodes.begin(), pointLightNodes.end(), [](SceneNode* a, SceneNode* b) {
			return GetPointLightIndex(a->GetName()) < GetPointLightIndex(b->GetName());
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

			light->SetIntensity(0.0f);

			this->lights.push_back(light);
			this->baseIntensities.push_back(3.0f);
		}

		this->lightsOnTime = Time::Current();
	}

	void Update() {
		for (int index = 0; index < this->lights.size(); index++) {
			Light* light = this->lights[index];
			float flicker = glm::sin(Time::Current() * (0.6f + (light->GetID() % 4) * 0.15f) + light->GetID() * 4.0f) * 0.5f;
			float targetIntensity = this->baseIntensities[index] + flicker;
			float turnOnAmount = glm::clamp((Time::Current() - this->lightsOnTime - float(index) * this->sequenceDelay) / this->fadeDuration, 0.0f, 1.0f);

			light->SetIntensity(targetIntensity * turnOnAmount);
		}
	}
};

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

inline void CollectNodesByPrefixRecursive(SceneNode* node, const std::string& prefix, std::vector<SceneNode*>& result) {
	if (!node) {
		return;
	}

	if (node->GetName().rfind(prefix, 0) == 0) {
		result.push_back(node);
	}

	for (SceneNode* child : node->GetChildren()) {
		CollectNodesByPrefixRecursive(child, prefix, result);
	}
}

inline SceneNode* FindFirstNodeByNameRecursive(SceneNode* node, const std::string& name) {
	if (!node) {
		return nullptr;
	}

	if (node->GetName() == name) {
		return node;
	}

	for (SceneNode* child : node->GetChildren()) {
		SceneNode* foundNode = FindFirstNodeByNameRecursive(child, name);

		if (foundNode != nullptr) {
			return foundNode;
		}
	}

	return nullptr;
}

inline SceneNode* FindTutorialDoorNode(SceneNode* roomNode) {
	for (const std::string& doorName : {"Exit Gate", "Door", "Doors", "Gate", "DOOR", "GATE"}) {
		SceneNode* doorNode = FindFirstNodeByNameRecursive(roomNode, doorName);

		if (doorNode != nullptr) {
			return doorNode;
		}
	}

	spdlog::warn("TutorialThrowingRoomScene: door node not found. Add its GLB node name to FindTutorialDoorNode().");

	return nullptr;
}

class TutorialThrowingRoomDoorOpener : public GameObject {
private:
	SceneNode* playerNode = nullptr;
	SceneNode* doorNode = nullptr;

	glm::vec3 closedPosition = glm::vec3(0.0f);
	bool initialized = false;
	bool opening = false;

	float openDistance = 3.0f;
	float openAmount = 4.0f;
	float openSpeed = 2.2f;

	void UpdateDoorPhysics() {
		if (this->doorNode == nullptr) {
			return;
		}

		for (Physics::Body* body : this->doorNode->GetAllObjectsInChildren<Physics::Body>()) {
			if (body != nullptr && body->GetNode() != nullptr) {
				body->SetPosition(body->GetNode()->GlobalTransform().Position());
			}
		}
	}

public:
	void Initialize(SceneNode* playerNode, SceneNode* doorNode) {
		this->playerNode = playerNode;
		this->doorNode = doorNode;

		if (this->doorNode != nullptr) {
			this->closedPosition = this->doorNode->GlobalTransform().Position().Value();
			this->initialized = true;
		}
	}

	void Update() {
		if (!this->initialized || this->playerNode == nullptr || this->doorNode == nullptr) {
			return;
		}

		if (TutorialStaticRatTarget::remainingRats > 0) {
			return;
		}

		if (!this->opening) {
			if (!PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
				return;
			}

			float distanceToDoor = glm::distance(
				this->playerNode->GlobalTransform().Position().Value(),
				this->doorNode->GlobalTransform().Position().Value()
			);

			if (distanceToDoor >= this->openDistance) {
				return;
			}

			this->opening = true;
		}

		glm::vec3 doorPosition = this->doorNode->GlobalTransform().Position().Value();
		float targetY = this->closedPosition.y - this->openAmount;

		if (doorPosition.y <= targetY) {
			return;
		}

		doorPosition.y = glm::max(targetY, doorPosition.y - Time::Delta() * this->openSpeed);
		this->doorNode->GlobalTransform().Position() = doorPosition;

		UpdateDoorPhysics();
	}
};

class TutorialThrowingRoomExitToCrafting : public GameObject {
private:
	bool sceneRequested = false;
	glm::vec3 triggerPosition = glm::vec3(19.8054f, 0.94866f, 0.01518f);
	float triggerRadius = 2.5f;

public:
	void Awake() {
		SceneNode* triggerNode = GetNode()->FindNode("NextRoom");

		if (triggerNode != nullptr) {
			this->triggerPosition = triggerNode->GlobalTransform().Position().Value();
		}
		else {
			spdlog::error("TutorialThrowingRoomExitToCrafting: NextRoom node not found. Using fallback trigger position.");
		}
	}

	void Update() {
		if (this->sceneRequested || !PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles") || PlayerController::Instance() == nullptr) {
			return;
		}

		float distanceToTrigger = glm::distance(
			PlayerController::Instance()->GlobalTransform().Position().Value(),
			this->triggerPosition
		);

		if (distanceToTrigger < this->triggerRadius) {
			this->sceneRequested = true;

			Application::Get()->RequestSceneBuild(
				[](Scene* s) { CraftingScene::InitScene(*s); }
			);
		}
	}
};

inline void AddRatModel(Scene& mainScene, SceneNode* ratNode) {
	SceneNode* ratModel = ResourceDatabase::Global->Get<GltfScene>("./res/models/enemy/rat6.glb")
		->Instantiate(&mainScene, ratNode, "RatModel");

	for (MeshRenderer* ratPart : ratModel->GetAllObjectsInChildren<MeshRenderer>()) {
		ratPart->maskFlags |= MaskEffectBits::Outline;
	}

	ratModel->LocalTransform().Position() = glm::vec3(0.0f, -0.45f, 0.0f);
	ratModel->LocalTransform().Scale() = glm::vec3(1.6f);
}

inline float FindNearestSurfaceY(Surface* surface, const glm::vec3& markerPosition) {
	if (surface == nullptr || surface->GetWalkablePoints().empty()) {
		return markerPosition.y;
	}

	float nearestY = markerPosition.y;
	float nearestDistanceSq = std::numeric_limits<float>::max();

	for (const glm::vec3& point : surface->GetWalkablePoints()) {
		float dx = point.x - markerPosition.x;
		float dz = point.z - markerPosition.z;
		float distanceSq = dx * dx + dz * dz;

		if (distanceSq < nearestDistanceSq) {
			nearestDistanceSq = distanceSq;
			nearestY = point.y;
		}
	}

	return nearestDistanceSq > 9.0f ? markerPosition.y : nearestY;
}

inline glm::vec3 GetRatSpawnPosition(SceneNode* spawnNode, Surface* surface) {
	glm::vec3 markerPosition = spawnNode->GlobalTransform().Position().Value();
	float floorY = FindNearestSurfaceY(surface, markerPosition);

	if (markerPosition.y > floorY + 0.5f) {
		floorY = markerPosition.y;
	}

	return glm::vec3(markerPosition.x, floorY + RatBodyCenterOffset, markerPosition.z);
}

inline void SpawnRatAt(Scene& mainScene, SceneNode* spawnNode, SceneNode* playerNode, Surface* surface) {
	glm::vec3 spawnPosition = GetRatSpawnPosition(spawnNode, surface);

	SceneNode* ratNode = mainScene.CreateNode("Static Tutorial Rat");
	ratNode->GlobalTransform().Position() = spawnPosition;

	JPH::ShapeRefC ratShape = new JPH::CapsuleShape(RatCapsuleHalfHeight, RatCapsuleRadius);

	JPH::BodyCreationSettings ratSettings(
		ratShape,
		JPH::RVec3(spawnPosition.x, spawnPosition.y, spawnPosition.z),
		JPH::Quat::sIdentity(),
		JPH::EMotionType::Dynamic,
		Physics::Layers::MOVING
	);

	Physics::Body* ratBody = ratNode->AddObject<Physics::Body>(ratSettings);
	ratBody->SetCollisionLayerAndMask({0}, 0xFFFFFFFF);

	AddRatModel(mainScene, ratNode);

	ratNode->AddObject<TutorialStaticRatTarget>()->Initialize(
		playerNode,
		10.0f,
		1.6f,
		1.0f
	);

	auto* skeleton = ratNode->AddObject<EnemySkeleton>();
	skeleton->SetTargetNode(playerNode);
	skeleton->SetSurface(surface);

	if (auto* fs = mainScene.GetComponent<FlockingSystem>()) {
		spdlog::error("reg");
		skeleton->RegisterToFlockingSystem(fs);
	}


	skeleton->OnPlayerEnteredRoom();
}

inline void SpawnTutorialRats(Scene& mainScene, SceneNode* roomNode, SceneNode* playerNode, Surface* surface) {
	std::vector<SceneNode*> ratSpawns;

	CollectNodesByPrefixRecursive(roomNode, "ENEMY_SPAWN_Rat", ratSpawns);

	std::sort(ratSpawns.begin(), ratSpawns.end(), [](SceneNode* a, SceneNode* b) {
		return a->GetName() < b->GetName();
	});

	if (ratSpawns.size() > MaxTutorialRatEnemies) {
		ratSpawns.resize(MaxTutorialRatEnemies);
	}

	for (SceneNode* spawnNode : ratSpawns) {
		SpawnRatAt(mainScene, spawnNode, playerNode, surface);
	}
}
