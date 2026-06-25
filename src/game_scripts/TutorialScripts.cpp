#include <game_scripts/TutorialScripts.h>

#include <Texture.h>
#include <text/Text3D.h>
#include <text/Font.h>
#include <TimeSystem.h>
#include <InputSystem.h>
#include <Light.h>
#include <Skybox.h>
#include <game_scripts/PlayerController.h>
#include <MeshRenderer.h>
#include <Random.h>
#include <physics/Body.h>
#include <game_scripts/PotionInventory.h>
#include <game_scripts/AttackEffects/EffectsManager.h>
#include <MathHelpers.h>
#include <Application.h>
#include <../game/include/scenes/CraftingScene.h>


template<class T_Loot>
void SpawnTutorialIngredientLoot(
    Scene* scene,
    const glm::vec3& position,
    const std::string& modelPath,
    const std::string& nodeName
) {
    SceneNode* node = SpawnIngredientLootModel(
        scene,
        position,
        modelPath,
        nodeName
    );

    node->AddObject<T_Loot>();
}

void SpawnTutorialRatMainEffectDrop(Scene* scene, const glm::vec3& centerPosition, int dropIndex) {
    if (!scene) {
        return;
    }

    glm::vec3 dropPosition = centerPosition + glm::vec3(0.0f, 0.35f, 0.0f);

    switch (dropIndex % 5) {
        case 0:
            SpawnTutorialIngredientLoot<LootSugar>(
                scene,
                dropPosition,
                "./res/models/ingredients/sugar.glb",
                "LootSugar"
            );
            return;

        case 1:
            SpawnTutorialIngredientLoot<LootBeetroot>(
                scene,
                dropPosition,
                "./res/models/ingredients/dried_beet.glb",
                "LootDriedBeet"
            );
            return;

        case 2:
            SpawnTutorialIngredientLoot<LootBone>(
                scene,
                dropPosition,
                "./res/models/ingredients/bone.glb",
                "LootBone"
            );
            return;

        case 3:
            SpawnTutorialIngredientLoot<LootCrystal>(
                scene,
                dropPosition,
                "./res/models/ingredients/water.glb",
                "LootWater"
            );
            return;

        default:
            SpawnTutorialIngredientLoot<LootRatTail>(
                scene,
                dropPosition,
                "./res/models/ingredients/rat_tail.glb",
                "LootRatTail"
            );
            return;
    }
}

inline void AddRatModel(Scene* mainScene, SceneNode* ratNode) {
	SceneNode* ratModel = ResourceDatabase::Global->Get<GltfScene>("./res/models/enemy/rat6.glb")
		->Instantiate(mainScene, ratNode, "RatModel");

	for (MeshRenderer* ratPart : ratModel->GetAllObjectsInChildren<MeshRenderer>()) {
		// ratPart->maskFlags |= MaskEffectBits::Outline;
		for (int materialIndex = 0; materialIndex < ratPart->GetMaterialCount(); materialIndex++) {
			ratPart->GetMaterial(materialIndex)->SetValue("ambientBump", 0.5f);
		}
	}

	ratModel->LocalTransform().Position() = glm::vec3(0.0f, 0.0f, 0.0f);
	ratModel->LocalTransform().Scale() = glm::vec3(1.6f);
}

int TutorialStaticRatTarget::remainingRats = 0;

void TutorialStaticRatTarget::Initialize(
	SceneNode* playerNode,
	float damage,
	float damageRange,
	float damageCooldown,
	int mainEffectDropIndex
) {
	this->playerNode = playerNode;
	this->damage = damage;
	this->damageRange = damageRange;
	this->damageCooldown = damageCooldown;
	this->mainEffectDropIndex = mainEffectDropIndex;
	this->damageTimer = 0.0f;

	this->myNode = GetNode();
	this->currentPos = GetNode()->GlobalTransform().Position().Value();
	this->m_hp = 100;
}

void TutorialStaticRatTarget::Awake() {
	this->myNode = GetNode();
	this->currentPos = GetNode()->GlobalTransform().Position().Value();

	remainingRats++;
}

void TutorialStaticRatTarget::Update() {
	glm::vec3 pos = GetNode()->GlobalTransform().Position();
	
	pos.y = 0;

	GetNode()->GlobalTransform().Position() = pos;
}

void TutorialStaticRatTarget::Die() {
	if (remainingRats > 0) {
		remainingRats--;
	}

	SceneNode* node = GetNode();
	Scene* scene = GetScene();

	if (node && scene) {
		glm::vec3 dropPosition = node->GlobalTransform().Position().Value();
		SpawnTutorialRatMainEffectDrop(scene, dropPosition, mainEffectDropIndex);

		scene->QueueDelete(node);
		myNode = nullptr;
	}

	if (m_Surface) {
		m_Surface->RemoveEnemy(this);
	}
}

void GateKey::OnPickUp() {
	this->tutorialScript->state = TutorialBaseScript::PickedUpKey;
}

void GateKey::DrawImGui() {
	Debug::Property(GetNode(), this->tutorialScript, "Tutorial Script");
}

void TutorialBottlePickup::OnPickUp() {
	PotionInventory::EnsureStartingIngredients();

	if (!PotionInventory::HasPotion()) {
		PotionInventory::SaveLastCraftedPotion(
			"Basic Potion",
			"Explosion",
			100.0f,
			999,
			false
		);
	}

	if (PlayerController::Instance()) {
		PlayerController::Instance()->SetThrowingUnlocked(true);
	}
}

void TutorialElevator::Update() {
	PlayerController* player = PlayerController::Instance();

	if (glm::distance(player->GlobalTransform().Position().WithY(0), GlobalTransform().Position().WithY(0)) < 1) {
		this->doorClosed = 0.001f;
	}

	if (this->doorClosed > 0) {
		glm::vec3 rot = GetNode()->FindNode("Drzwi")->LocalTransform().Rotation().EulerAngles();

		rot.x = 0;
		rot.y = Math::MoveTowards(rot.y, 0, Time::Delta());
		rot.z = 0;

		GetNode()->FindNode("Drzwi")->LocalTransform().Rotation() = rot;

		if (rot.y == 0) {
			GlobalTransform().Position().SetY(
				Math::MoveTowards(GlobalTransform().Position().y, -10, Time::Delta() * 3)
			);
			
			player->SetPosition(GlobalTransform().Position());

			if (GlobalTransform().Position().y <= -5) {
				PotionInventory::ClearTutorialPotions();
				Application::Get()->RequestSceneBuild(
					[](Scene* s) { CraftingScene::InitScene(*s); }
				);
			}
		}
	}
}

void TutorialElevator::DrawImGui() {

}

void TutorialLights::DrawImGui() {
	Debug::Property(lightIntensity, "lightIntensity");

	Debug::Property(GetNode(), alwaysOnLights, "alwaysOnLights");
	Debug::Property(GetNode(), firstRoomGradualLights, "firstRoomGradualLights");
	Debug::Property(GetNode(), corridorLights, "corridorLights");
	Debug::Property(GetNode(), secondRoomLights, "secondRoomLights");
	Debug::Property(GetNode(), elevatorRoomLights, "elevatorRoomLights");
}

void TutorialLights::Update() {
	for (Light* l : this->alwaysOnLights) {
		l->SetIntensity(this->lightIntensity + glm::sin(l->GetID() + Time::Current()) * 3);
	}

	if (this->fireGradualLights) {
		for (int i = 0; i < this->firstRoomGradualLights.size(); i++) {
			this->firstRoomGradualLights[i]->SetIntensity((this->lightIntensity + glm::sin(i + Time::Current()) * 3) * glm::clamp(this->gradualLightsState - i, 0.f, 1.f));
		}

		this->gradualLightsState += Time::Delta() / 0.5f;
	}
	else {
		for (Light* l : this->firstRoomGradualLights) {
			l->SetIntensity(0);
		}
	}

	if (this->fireCorridorLights) {
		for (int i = 0; i < this->corridorLights.size(); i += 2) {
			this->corridorLights[i]->SetIntensity((this->lightIntensity + glm::sin(i + Time::Current()) * 3) * glm::clamp(this->corridorLightsState - i, 0.f, 1.f));
			this->corridorLights[i + 1]->SetIntensity((this->lightIntensity + glm::sin(i + Time::Current()) * 3) * glm::clamp(this->corridorLightsState - i, 0.f, 1.f));
		}

		corridorLightsState += Time::Delta();
	}
	else {
		for (Light* l : this->corridorLights) {
			l->SetIntensity(0);
		}
	}

	PlayerController* player = PlayerController::Instance();

	this->maxPlayerProgress = glm::max(
		this->maxPlayerProgress,
		player->GlobalTransform().Position().z
	);

	for (int i = 0; i < this->secondRoomLights.size(); i++) {
		this->secondRoomLights[i]->SetIntensity(
			(this->lightIntensity + glm::sin(i + Time::Current()) * 3)
			*
			glm::clamp(this->maxPlayerProgress - this->secondRoomLights[i]->GlobalTransform().Position().z + 6, 0.f, 1.f)
		);
	}
}

void TutorialBaseScript::Awake() {
	// this->gate = GetNode()->FindNode("Exit Gate");
	// this->key = GetNode()->FindNode("Gate Key");

	// assert(this->gate != nullptr);
	// assert(this->key != nullptr);

	// this->key->AddObject<GateKey>();

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

	SceneNode* text3dNode = GetScene()->GetOrCreateNode("Text 3D");
	text3dNode->LocalTransform().Position() = {3.0f, 2.0f, 1.0f};
	text3dNode->GlobalTransform().Scale() = glm::vec3(0.5f);
	this->tutorialText = text3dNode->AddObjectIfMissing<Text3D>(" ", papyrusFont);
	this->tutorialText->color = {1.2f, 0.1f, 0.0f, 0.0f};
	this->tutorialText->billboardMode = BillboardMode::Enabled;
	this->tutorialText->SetAlignment(TextAlignment::Middle);

	// this->timePoint = Time::Current();

    ShaderProgram* skyProg = ShaderProgram::Build()
	.WithVertexShader(("./res/shaders/skybox.vert"))
	.WithPixelShader(("./res/shaders/skybox.frag"))
	.Link();

    Cubemap* skyCubemap = GetScene()->Resources()->Get<Cubemap>(
        "./res/textures/null_skybox.hdr", Texture::HDRColorBuffer);
    skyCubemap->SetWrapModeU(TextureWrap::Clamp);
    skyCubemap->SetWrapModeV(TextureWrap::Clamp);
    skyCubemap->SetWrapModeW(TextureWrap::Clamp);

    Material* skyMat = new Material(skyProg);
    skyMat->SetValue("skyboxTexture", skyCubemap);

	Skybox* sky = GetNode()->AddObjectIfMissing<Skybox>(skyMat);
	
	this->state = MovingTip;
	this->timePoint = -2;
}

void TutorialBaseScript::Update() {
	PlayerController* player = PlayerController::Instance();

	if (glm::isnan(this->playerStartPos.x)) {
		this->playerStartPos = player->GlobalTransform().Position();
	}

	if (this->state == MovingTip) {
		if (glm::distance(player->GlobalTransform().Position().Value(), this->playerStartPos) > 1) {
			this->state = MovingToDoors;
			
			this->timePoint = 0;

			this->tutorialText->color.w = 0;
		}
		else {
			this->timePoint += Time::Delta() / 1.5f;

			this->tutorialText->GlobalTransform().Position() = this->movementPromptNode->GlobalTransform().Position();
	
			this->tutorialText->SetText("Use WASD to move around");

			this->tutorialText->color.w = (1 - glm::distance(player->GlobalTransform().Position().Value(), this->playerStartPos)) * glm::clamp(this->timePoint, 0.f, 1.f);
		}
	}
	if (this->state == MovingToDoors || this->state == ReachedDoorsTip) {
		float distToDoors = glm::distance(player->GlobalTransform().Position().Value(), this->gate->GlobalTransform().Position().Value());

		this->tutorialText->GlobalTransform().Position() = this->gatePromptNode->GlobalTransform().Position();
	
		this->tutorialText->SetText("The gate is locked, you have\nto find a key to open it");

		this->tutorialText->color.w = glm::clamp(4 - distToDoors, 0.f, 1.f);

		if (distToDoors < 3.2f) {
			this->lights->fireGradualLights = true;

			this->state = ReachedDoorsTip;
		}

		if (this->state == ReachedDoorsTip && distToDoors > 4) {
			this->state = MovedAwayFromDoorsTip;
		}
	}
	if (this->state == MovedAwayFromDoorsTip) {
		float distToRotationPrompt = glm::distance(player->GlobalTransform().Position().Value(), this->rotationPromptNode->GlobalTransform().Position().Value());

		this->tutorialText->GlobalTransform().Position() = this->rotationPromptNode->GlobalTransform().Position();
	
		this->tutorialText->SetText("Use Q+E to rotate the camera");

		this->tutorialText->color.w = glm::clamp(4 - distToRotationPrompt, 0.f, 1.f);

		if (glm::distance(player->GlobalTransform().Position().Value(), this->key->GlobalTransform().Position().Value()) < 2) {
			this->state = GotCloseToKeyTip;
		}
	}
	if (this->state == GotCloseToKeyTip) {
		float distToKey = glm::distance(player->GlobalTransform().Position().Value(), this->key->GlobalTransform().Position().Value());

		this->tutorialText->GlobalTransform().Position() = this->keyPrompt->GlobalTransform().Position();
	
		this->tutorialText->SetText("Press F to pick up the key");

		this->tutorialText->color.w = glm::clamp(2 - distToKey, 0.f, 1.f);
	}
	if (this->state == PickedUpKey) {
		MeshRenderer* gateMesh = this->gate->GetObject<MeshRenderer>();

		this->tutorialText->color.w = 0;

		gateMesh->maskFlags |= MaskEffectBits::Outline;

		float distToDoors = glm::distance(player->GlobalTransform().Position().Value(), this->gate->GlobalTransform().Position().Value());

		if (distToDoors < 2) {
			this->state = Corridor;

			gateMesh->maskFlags &= ~MaskEffectBits::Outline;
		}
	}
	if (this->state == Corridor) {
		glm::vec3 gatePos = this->gate->GlobalTransform().Position();

		if (gatePos.y < 4) {
			gatePos.y += Time::Delta() * 2;

			this->gate->GlobalTransform().Position() = gatePos;
		}
		else {
			this->gate->GetAllObjectsInChildren<Physics::Body>()[0]->SetPosition(this->gate->GlobalTransform().Position());
		}

		this->lights->fireCorridorLights = true;

		float playerZ = player->GlobalTransform().Position().z;
		float promptZ = this->roadBlockedPrompt->GlobalTransform().Position().z;

		float distZ = abs(playerZ - promptZ);

		if (distZ < 2) {
			this->tutorialText->GlobalTransform().Position() = this->roadBlockedPrompt->GlobalTransform().Position();
		
			this->tutorialText->SetText("The road is blocked.\nFind a way to clear it");
	
			this->tutorialText->color.w = glm::clamp(2 - distZ, 0.f, 1.f);
		}
		else {
			this->tutorialText->GlobalTransform().Position() = this->bottlesPickupPrompt->GlobalTransform().Position();
		
			this->tutorialText->SetText("Pick up explosive potions");
	
			this->tutorialText->color.w = 1;
		}

		if (PotionInventory::HasPotion()) {
			this->state = RatFight;
		}
	}
	if (this->state == RatFight) {
		if (!this->ratsSpawned) {
			std::vector<int> ratIndices(this->ratSpawns.size());

			for (int i = 0; i < ratIndices.size(); i++) {
				ratIndices[i] = i;
			}

			this->ratRoomGate->GlobalTransform().Position().SetY(1.23177);
			this->ratRoomGate->GetAllObjectsInChildren<Physics::Body>()[0]->SetPosition(this->ratRoomGate->GlobalTransform().Position());

			ratIndices = Random::RandomShuffle(ratIndices);

			for (int i = 0; i < glm::min(6, (int) ratIndices.size()); i++) {
				SceneNode* rat = GetScene()->CreateNode(std::format("Rat {}", i));

				AddRatModel(GetScene(), rat);

				auto* ratEnemy = rat->AddObject<TutorialStaticRatTarget>();
				
				ratEnemy->Initialize(
					player->GetNode(),
					10.0f,
					1.6f,
					1.0f,
					i
				);

				ratEnemy->m_hp = 1;

				rat->GlobalTransform().Position() = this->ratSpawns[ratIndices[i]]->GlobalTransform().Position();
			}

			this->ratsSpawned = true;
		}

		this->tutorialText->GlobalTransform().Position() = player->GlobalTransform().Position().WithY(2);
		
		this->tutorialText->SetText("Defeat all rats to leave the room");

		this->tutorialText->color.w = 1;

		if (TutorialStaticRatTarget::remainingRats <= 0) {
			this->state = SmashCrates;
		}
	}
	if (this->state == SmashCrates) {
		glm::vec3 gatePos = this->ratRoomGate->GlobalTransform().Position();

		if (gatePos.y < 4) {
			gatePos.y += Time::Delta() * 2;

			this->ratRoomGate->GlobalTransform().Position() = gatePos;
		}
		else {
			this->ratRoomGate->GetAllObjectsInChildren<Physics::Body>()[0]->SetPosition(this->ratRoomGate->GlobalTransform().Position());
		}

		float playerZ = player->GlobalTransform().Position().z;
		float promptZ = this->roadBlockedPrompt->GlobalTransform().Position().z;

		float distZ = abs(playerZ - promptZ);

		this->tutorialText->GlobalTransform().Position() = this->roadBlockedPrompt->GlobalTransform().Position();
		
		this->tutorialText->SetText("Smash crates with the explosive potion");

		this->tutorialText->color.w = glm::clamp(4 - distZ, 0.f, 1.f);

		auto explosions = GetScene()->FindObjectsOfType<EffectExplosion>();

		if (explosions.size() > 0 && glm::distance(explosions[0]->GlobalTransform().Position().WithY(0), this->roadBlockedPrompt->GlobalTransform().Position().WithY(0)) < 3) {
			for (auto* bit : this->crateBits) {
				delete bit;
			}

			this->state = Elevator;
		}
	}
}

void TutorialBaseScript::DrawImGui() {
	Debug::Property(GetNode(), this->key, "Gate Key");
	Debug::Property(GetNode(), this->lights, "Lights Node");
	Debug::Property(GetNode(), this->gate, "Gate Node");
	Debug::Property(GetNode(), this->ratRoomGate, "Rat Room Gate Node");
	Debug::Property(GetNode(), this->movementPromptNode, "Movement Prompt");
	Debug::Property(GetNode(), this->gatePromptNode, "Gate Prompt");
	Debug::Property(GetNode(), this->rotationPromptNode, "Rotation Prompt");
	Debug::Property(GetNode(), this->keyPrompt, "Key Prompt");
	Debug::Property(GetNode(), this->roadBlockedPrompt, "Road Blocked Prompt");
	Debug::Property(GetNode(), this->bottlesPickupPrompt, "Bottles Pickup Prompt");
	
	Debug::Property(GetNode(), this->ratSpawns, "Rat Spawns");
	Debug::Property(GetNode(), this->crateBits, "Crate Bits");
}
