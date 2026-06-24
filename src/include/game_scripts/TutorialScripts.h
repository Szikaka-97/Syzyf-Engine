#pragma once

#include <GameObject.h>
#include <game_scripts/PickableItem.h>
#include <Debug.h>
#include <game_scripts/enemies/EnemySkeleton.h>

class Text3D;
class TutorialBaseScript;

class GateKey : public PickableItem, public ImGuiDrawable {
public:
	serialized TutorialBaseScript* tutorialScript;

	GateKey() = default;

	virtual void OnPickUp() override;

	virtual void DrawImGui() override;
};


class TutorialBottlePickup : public PickableItem {
public:
	TutorialBottlePickup() = default;

	virtual void OnPickUp() override;
};

class TutorialElevator : public GameObject, public ImGuiDrawable {
public:
	float doorClosed;

	TutorialElevator() = default;

	void Update();

	virtual void DrawImGui() override;
};

class TutorialLights : public GameObject, public ImGuiDrawable {
public:
	serialized TutorialBaseScript* tutorialScript;

	serialized float lightIntensity = 10;

	serialized std::vector<Light*> alwaysOnLights;
	serialized std::vector<Light*> firstRoomGradualLights;
	serialized std::vector<Light*> corridorLights;
	serialized std::vector<Light*> secondRoomLights;
	serialized std::vector<Light*> elevatorRoomLights;

	bool fireGradualLights = false;
	float gradualLightsState = 0;
	bool fireCorridorLights = false;
	float corridorLightsState = 0;
	float maxPlayerProgress = 0;
	
	TutorialLights() = default;

	void Update();
	
	virtual void DrawImGui() override;
};

class TutorialBaseScript : public GameObject, public ImGuiDrawable {
public:
	enum TutorialState {
		MovingTip,
		MovingToDoors,
		ReachedDoorsTip,
		MovedAwayFromDoorsTip,
		GotCloseToKeyTip,
		PickedUpKey,
		Corridor,
		RatFight,
		SmashCrates,
		Elevator
	};

	serialized Text3D* tutorialText;
	float timePoint = 0;
	glm::vec3 playerStartPos = glm::vec3(NAN);
	TutorialState state = MovingTip;
	bool ratsSpawned = false;
	serialized GateKey* key;
	serialized TutorialLights* lights;
	serialized SceneNode* gate;
	serialized SceneNode* ratRoomGate;
	serialized SceneNode* movementPromptNode;
	serialized SceneNode* gatePromptNode;
	serialized SceneNode* rotationPromptNode;
	serialized SceneNode* keyPrompt;
	serialized SceneNode* roadBlockedPrompt;
	serialized SceneNode* bottlesPickupPrompt;
	serialized std::vector<SceneNode*> ratSpawns;
	serialized std::vector<SceneNode*> crateBits;
public:
	TutorialBaseScript() = default;

	void Awake();

	void Update();

	virtual void DrawImGui();
};

class TutorialStaticRatTarget : public EnemySkeleton {
private:
	SceneNode* playerNode = nullptr;

	int mainEffectDropIndex = 0;

	float damage = 10.0f;
	float damageRange = 1.6f;
	float damageCooldown = 1.0f;
	float damageTimer = 0.0f;

public:
	static int remainingRats;

	void Initialize(
		SceneNode* playerNode,
		float damage,
		float damageRange,
		float damageCooldown,
		int mainEffectDropIndex
	);

	void Awake();

	void Update();

	virtual void Die() override;

	LootPool& GetLootPool() override {
		return LootPool::GetSkeletonLootPool();
	}
};
