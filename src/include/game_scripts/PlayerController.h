#pragma once

#include <GameObject.h>
#include <Debug.h>

namespace Physics {
	class VirtualCharacterController;
	class System;
};

class Camera;
class PickableItem;
class PickableItemSystem;

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

	float health = 100;

	float aimBearing;
	glm::quat defaultThrowingArmRotation;

	float throwStrengthCache = 0;
	float throwStrengthAccum = 0;

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

	void HandleItemInteractions();
public:
	static inline PlayerController* Instance() {
		return instance;
	}
	
	void Awake();
	void Update();

	void TakeDamage(float damage);
	
	float GetHealth() const;
	void SetHealth(float newHealth);

	void Die();

	inline bool CanThrow() const {
		return this->aim != nullptr;
	}

	virtual void DrawImGui() override;
};
