#pragma once

#include <Scene.h>

#include <Debug.h>
#include <TweenSystem.h>
#include <physics/System.h>
#include <animation/AnimationSystem.h>
#include <animation/AnimationComponent.h>
#include <ui/systems/UiSystem.h>
#include <ui/widgets/wheel/UiWheel.h>
#include <game_scripts/PickableItem.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/AimCrosshair.h>
#include <game_scripts/enemies/EnemySkeleton.h>

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
#include <text/Text3D.h>
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

class TutorialEnemyTouchDamage : public GameObject {
private:
	SceneNode* targetNode = nullptr;
	float damage = 15.0f;
	float attackRange = 1.35f;
	float attackCooldown = 1.0f;
	float attackTimer = 0.0f;

public:
	void Initialize(SceneNode* targetNode, float damage, float attackRange, float attackCooldown) {
		this->targetNode = targetNode;
		this->damage = damage;
		this->attackRange = attackRange;
		this->attackCooldown = attackCooldown;
		this->attackTimer = 0.0f;
	}

	void Update() {
		if (!PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
			return;
		}

		if (this->targetNode == nullptr) {
			return;
		}

		if (this->attackTimer > 0.0f) {
			this->attackTimer -= Time::Delta();
		}

		float distanceToPlayer = glm::distance(
			GetNode()->GlobalTransform().Position().Value(),
			this->targetNode->GlobalTransform().Position().Value()
		);

		if (distanceToPlayer <= this->attackRange && this->attackTimer <= 0.0f) {
			if (auto* player = this->targetNode->GetObject<PlayerController>()) {
				player->TakeDamage(this->damage);
				this->attackTimer = this->attackCooldown;

				spdlog::info("TutorialEnemyTouchDamage: player took {:.1f} damage", this->damage);
			}
		}
	}
};

class TutorialThrowingRoomCombatManager : public GameObject {
private:
	bool enemiesActivated = false;
	bool roomCleared = false;

	void ActivateEnemies() {
		for (EnemyBase* enemy : GetScene()->FindObjectsOfType<EnemyBase>()) {
			if (enemy != nullptr && enemy->myNode != nullptr) {
				enemy->OnPlayerEnteredRoom();
			}
		}

		this->enemiesActivated = true;
		spdlog::info("TutorialThrowingRoomCombatManager: enemies activated.");
	}

public:
	void Awake() {
		this->roomCleared = PersistentData::Get<bool>("TutorialThrowingRoom_RoomCleared");

		if (PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles") && !this->roomCleared) {
			ActivateEnemies();
		}
	}

	void Update() {
		if (this->roomCleared) {
			return;
		}

		if (!this->enemiesActivated) {
			if (PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles")) {
				ActivateEnemies();
			}

			return;
		}

		int enemiesAlive = 0;

		for (EnemyBase* enemy : GetScene()->FindObjectsOfType<EnemyBase>()) {
			if (enemy != nullptr && enemy->myNode != nullptr) {
				enemiesAlive++;
			}
		}

		if (enemiesAlive == 0) {
			this->roomCleared = true;
			PersistentData::Set<bool>("TutorialThrowingRoom_RoomCleared", true);

			spdlog::info("TutorialThrowingRoomCombatManager: room cleared.");
		}
	}
};

class TutorialThrowingPromptManager : public GameObject {
private:
	Text3D* promptText = nullptr;
	SceneNode* bottlesNode = nullptr;

	bool playerTookBottles = false;
	bool playerStartedAiming = false;
	bool playerReleasedThrow = false;
	bool roomCleared = false;

	float timePoint = 0.0f;

	float TimeSincePoint() const {
		return Time::Current() - this->timePoint;
	}

	void ShowPrompt(const std::string& text, const glm::vec3& position) {
		this->promptText->SetText(text);
		this->promptText->GlobalTransform().Position() = position;
		this->promptText->color.w = glm::clamp(this->promptText->color.w + Time::Delta() * 2.0f, 0.0f, 1.0f);
	}

public:
	void Awake() {
		this->bottlesNode = GetNode()->FindNode("Bottles");
		this->playerTookBottles = PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles");
		this->roomCleared = PersistentData::Get<bool>("TutorialThrowingRoom_RoomCleared");

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

		SceneNode* text3dNode = GetScene()->CreateNode("Throwing Tutorial Text 3D");
		text3dNode->GlobalTransform().Scale() = glm::vec3(0.35f);

		this->promptText = text3dNode->AddObject<Text3D>(" ", papyrusFont);
		this->promptText->color = {1.2f, 0.3f, 0.0f, 0.0f};
		this->promptText->billboardMode = BillboardMode::Enabled;
		this->promptText->SetAlignment(TextAlignment::Middle);

		this->timePoint = Time::Current();
	}

	void Update() {
		if (this->promptText == nullptr) {
			return;
		}

		bool tookBottlesNow = PersistentData::Get<bool>("TutorialThrowingRoom_PlayerTookBottles");
		bool roomClearedNow = PersistentData::Get<bool>("TutorialThrowingRoom_RoomCleared");

		if (tookBottlesNow && !this->playerTookBottles) {
			this->playerTookBottles = true;
			this->timePoint = Time::Current();
		}

		if (roomClearedNow && !this->roomCleared) {
			this->roomCleared = true;
			this->timePoint = Time::Current();
		}

		if (!this->playerTookBottles) {
			glm::vec3 promptPosition = glm::vec3(0.0f, 2.0f, 0.0f);

			if (this->bottlesNode != nullptr) {
				promptPosition = this->bottlesNode->GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.2f, 0.0f);
			}
			else if (PlayerController::Instance()) {
				promptPosition = PlayerController::Instance()->GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.8f, 0.0f);
			}

			ShowPrompt("Find the bottles\nPress G to pick them up", promptPosition);

			return;
		}

		if (PlayerController::Instance() == nullptr) {
			return;
		}

		glm::vec3 playerPromptPosition = PlayerController::Instance()->GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.8f, 0.0f);

		if (this->roomCleared) {
			ShowPrompt("The room is clear\nReturn to the gate", playerPromptPosition);
			return;
		}

		if (!this->playerStartedAiming) {
			ShowPrompt("Hold LMB to aim\nRelease to throw a bottle", playerPromptPosition);

			if (GetScene()->Input()->ButtonPressed(0)) {
				this->playerStartedAiming = true;
				this->timePoint = Time::Current();
			}

			return;
		}

		if (!this->playerReleasedThrow) {
			if (GetScene()->Input()->ButtonPressed(0)) {
				ShowPrompt("Release LMB\nto throw", playerPromptPosition);
			}
			else {
				this->playerReleasedThrow = true;
				this->timePoint = Time::Current();
			}

			return;
		}

		ShowPrompt("Hit the rats\nwith bottles", playerPromptPosition);
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

inline void AddEnemyModel(Scene& mainScene, SceneNode* enemyNode, const std::string& modelPath, const glm::vec3& scale) {
	SceneNode* enemyModel = ResourceDatabase::Global->Get<GltfScene>(modelPath)
		->Instantiate(&mainScene, enemyNode, "EnemyModel");

	enemyModel->LocalTransform().Position() = glm::vec3(0.0f);
	enemyModel->GlobalTransform().Scale() = scale;

	if (auto* animComp = enemyModel->GetObjectInChildren<AnimationComponent>()) {
		if (auto* enemy = enemyNode->GetObject<EnemyBase>()) {
			enemy->SetAttackAnimation(animComp);
		}
	}
}

inline EnemySkeleton* SpawnRatEnemyAt(Scene& mainScene, SceneNode* spawnNode, SceneNode* playerNode, Surface* surface) {
	glm::vec3 spawnPosition = spawnNode->GlobalTransform().Position().Value();

	SceneNode* enemyNode = mainScene.CreateNode("Enemy Rat");
	enemyNode->GlobalTransform().Position() = spawnPosition;

	JPH::ShapeRefC enemyShape = new JPH::CapsuleShape(0.35f, 0.7f);

	JPH::BodyCreationSettings enemySettings(
		enemyShape,
		JPH::RVec3(spawnPosition.x, spawnPosition.y + 0.8f, spawnPosition.z),
		JPH::Quat::sIdentity(),
		JPH::EMotionType::Dynamic,
		Physics::Layers::MOVING
	);

	Physics::Body* enemyBody = enemyNode->AddObject<Physics::Body>(enemySettings);
	enemyBody->SetRestitution(0.0f);
	enemyBody->SetFriction(0.8f);
	enemyBody->SetCollisionLayerAndMask({1}, {0});

	EnemySkeleton* enemyAi = enemyNode->AddObject<EnemySkeleton>();
	enemyAi->SetTargetNode(playerNode);
	enemyAi->SetAttackCooldown(1.2f);
	enemyAi->attackRange = 1.5f;

	if (surface != nullptr) {
		enemyAi->SetSurface(surface);
		enemyAi->SetRoomID(surface->GetID());
		surface->AddEnemy(enemyAi);
	}

	AddEnemyModel(
		mainScene,
		enemyNode,
		"./res/models/enemy/rat6.glb",
		glm::vec3(1.0f)
	);

	enemyNode->AddObject<TutorialEnemyTouchDamage>()->Initialize(
		playerNode,
		15.0f,
		1.35f,
		1.0f
	);

	return enemyAi;
}

inline void SpawnTutorialEnemies(Scene& mainScene, SceneNode* roomNode, SceneNode* playerNode, Surface* surface) {
	if (PersistentData::Get<bool>("TutorialThrowingRoom_RoomCleared")) {
		spdlog::info("TutorialThrowingRoomScene: room already cleared, skipping enemies.");
		return;
	}

	std::vector<SceneNode*> ratSpawns;

	CollectNodesByPrefixRecursive(roomNode, "ENEMY_SPAWN_Rat", ratSpawns);

	for (SceneNode* spawnNode : ratSpawns) {
		SpawnRatEnemyAt(mainScene, spawnNode, playerNode, surface);
	}

	spdlog::info(
		"TutorialThrowingRoomScene: spawned rat enemies={}",
		ratSpawns.size()
	);
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

	AddRoomLights(roomNode);

	SetupBottlePickup(roomNode);

	roomNode->AddObject<TutorialThrowingPromptManager>();
	roomNode->AddObject<TutorialThrowingRoomCombatManager>();
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

	SpawnTutorialEnemies(mainScene, roomNode, playerNode, surface);
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