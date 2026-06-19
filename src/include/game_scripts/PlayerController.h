#pragma once

#include <GameObject.h>
#include <Debug.h>

#include <string>

namespace Physics {
class VirtualCharacterController;
class System;
};

class Camera;
class PickableItem;
class PickableItemSystem;
class AnimationComponent;

class PlayerController : public GameObject, public ImGuiDrawable {
private:
	static PlayerController* instance;

	float wobliness = 0;
	float woblinessFrequency = 1;
	float speed = 5;
	float bodyDragTime = 0.5;
	float aimSpeed = 2;

	float throwSpeedTime = 0.6;
	float minThrowDistance = 1;
	float maxThrowDistance = 5;
	float flightTime = 1;
	float velocityThrowBoost = 0.2;

	std::queue<glm::vec4> prevPositions;
	Physics::VirtualCharacterController* charController = nullptr;
	SceneNode* torso = nullptr;
	SceneNode* aim = nullptr;
	SceneNode* characterRoot = nullptr;
	SceneNode* throwingArm = nullptr;
	SceneNode* throwPoint = nullptr;
	SceneNode* rightHand = nullptr;

	AnimationComponent* animator = nullptr;
	std::string walkAnimationName;
	std::string throwAnimationName;
	std::string activeLoopAnimationName;
	float throwAnimationTimer = 0.0f;
	float throwAnimationDuration = 0.0f;
	float movementAmount = 0.0f;
	bool throwAnimationActive = false;

	float health = 100;

	float aimBearing;
	glm::quat defaultThrowingArmRotation;

	float throwStrengthCache = 0;
	float throwStrengthAccum = 0;

	bool throwingUnlocked = true;

	float itemHighlightRadius = 2.0f;

	glm::vec3 GetMousePointOnGround(Camera* camera);
	glm::vec3 GetStrengthFromVelocity();

	PickableItem* highlightedItem = nullptr;
	//  Cached systems
	PickableItemSystem* pickableItemSystem = nullptr;
	Physics::System* physics = nullptr;

	void UpdateMovement();
	void UpdateTargetting();
	void UpdateThrowing();
	void UpdatePlayerAnimation();

	void SetupCharacterNodes();
	void SetupAnimations();
	void PlayLoopAnimation(const std::string& animationName);
	void StopLoopAnimation();
	void PlayThrowAnimation();
	void StopAnimation(const std::string& animationName, bool resetToStart);

	void HandleItemInteractions();
public:
	static inline PlayerController* Instance() {
		return instance;
	}

	PlayerController() = default;

	void Awake();
	void Update();
	void OnEnable();
	void OnDisable();

	void TakeDamage(float damage);

	float GetHealth() const;
	void SetHealth(float newHealth);

	void Die();

	bool CanThrow() const;

	inline void SetThrowingUnlocked(bool unlocked) {
		this->throwingUnlocked = unlocked;

		if (!unlocked && this->aim != nullptr) {
			this->aim->SetEnabled(false);
		}
	}

	inline bool IsThrowingUnlocked() const {
		return this->throwingUnlocked;
	}

	virtual void DrawImGui() override;
};