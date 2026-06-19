#include "game_scripts/PlayerController.h"

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <vector>

#include <InputSystem.h>
#include <TimeSystem.h>
#include <Camera.h>
#include <Graphics.h>
#include <MathHelpers.h>
#include <game_scripts/ThrowableObjectPool.h>
#include <game_scripts/AttackEffects/EffectsManager.h>
#include <game_scripts/AttackEffects/combos/ComboExplodeFire.h>
#include <game_scripts/ThrowableObject.h>
#include <physics/VirtualCharacterController.h>
#include <physics/Body.h>
#include <Formatters.h>
#include <game_scripts/PickableItemSystem.h>
#include <game_scripts/PotionInventory.h>
#include <physics/LayerMaskFilter.h>
#include <animation/AnimationComponent.h>

PlayerController* PlayerController::instance;

float MoveTowards(float current, float target, float maxDelta) {
	maxDelta = glm::abs(maxDelta);

	if (current < target) {
		current += maxDelta;

		if (current > target) {
			return target;
		}
		else {
			return current;
		}
	}
	else {
		current -= maxDelta;

		if (current < target) {
			return target;
		}
		else {
			return current;
		}
	}
}

float MoveTowardsAngle(float current, float target, float maxDelta) {
	current = glm::mod(current, glm::tau<float>());
	target = glm::mod(target, glm::tau<float>());

	maxDelta = glm::abs(maxDelta);

	if (glm::abs(target - current) <= glm::pi<float>()) {
		return MoveTowards(current, target, maxDelta);
	}
	else {
		if (current < target) {
			current += glm::tau<float>();
		}
		else {
			target += glm::tau<float>();
		}

		current = MoveTowards(current, target, maxDelta);

		return glm::mod(current, glm::tau<float>());
	}
}

float ThrowStrengthEasing(float strength) {
	return -(glm::cos(glm::pi<float>() * strength) - 1) / 2;
}


namespace{
	float SecondaryEffectMultiplier(bool secondaryEffect){
		return secondaryEffect ? 0.65f : 1.0f;
	}

	EffectBase* AddPotionEffectToNode(
		SceneNode* node,
		const std::string& effectId,
		const Crafting::CraftedPotionData& potionData,
		bool secondaryEffect
	) {
		float multiplier = SecondaryEffectMultiplier(secondaryEffect);

		// if (effectId == Crafting::EffectId::Fire || effectId == Crafting::EffectId::Burn) {
			EffectFire* effect = node->AddObject<EffectFire>();

			effect->radius = potionData.radius * multiplier;
			effect->damage = potionData.power * multiplier;
			effect->dotRemainingTime = potionData.duration * multiplier;
			effect->ingredientCount = potionData.mainEffectCount;
			effect->special1 = potionData.modifierCount > 0;
			effect->special2 = potionData.modifierCount > 1;

			return effect;
		// }

		// if (effectId == Crafting::EffectId::Petrify) {
		// 	EffectPetrify* effect = node->AddObject<EffectPetrify>();

		// 	effect->radius = potionData.radius * multiplier;
		// 	effect->petrifyRemainingTime = potionData.duration * multiplier;
		// 	effect->ingredientCount = potionData.mainEffectCount;
		// 	effect->special1 = potionData.modifierCount > 0;
		// 	effect->special2 = potionData.modifierCount > 1;

		// 	return effect;
		// }

		// if (effectId == Crafting::EffectId::Tornado) {
		// 	EffectTornado* effect = node->AddObject<EffectTornado>();

		// 	effect->radius = potionData.radius * multiplier;
		// 	effect->damage = potionData.power * multiplier;
		// 	effect->tornadoRemainingTime = potionData.duration * multiplier;
		// 	effect->ingredientCount = potionData.mainEffectCount;
		// 	effect->special1 = potionData.modifierCount > 0;
		// 	effect->special2 = potionData.modifierCount > 1;

		// 	return effect;
		// }

		// if (effectId == Crafting::EffectId::Confuse) {
		// 	EffectConfuse* effect = node->AddObject<EffectConfuse>();

		// 	effect->radius = potionData.radius * multiplier;
		// 	effect->damage = static_cast<int>(potionData.power * multiplier);
		// 	effect->confuseRemainingTime = potionData.duration * multiplier;
		// 	effect->ingredientCount = potionData.mainEffectCount;
		// 	effect->special1 = potionData.modifierCount > 0;
		// 	effect->special2 = potionData.modifierCount > 1;

		// 	return effect;
		// }

		// if (effectId == Crafting::EffectId::Explosion || effectId == Crafting::EffectId::Lightning) {
		// 	EffectExplosion* effect = node->AddObject<EffectExplosion>();

		// 	effect->strength = 1.0f;
		// 	effect->maxRange = potionData.radius * multiplier;
		// 	effect->maxDamage = potionData.power * multiplier;
		// 	effect->explosionDuration = potionData.duration * multiplier;
		// 	effect->ingredientCount = potionData.mainEffectCount;
		// 	effect->special1 = potionData.modifierCount > 0;
		// 	effect->special2 = potionData.modifierCount > 1;

		// 	return effect;
		// }

		// EffectExplosion* effect = node->AddObject<EffectExplosion>();

		// effect->strength = 1.0f;
		// effect->maxRange = potionData.radius * multiplier;
		// effect->maxDamage = potionData.power * multiplier;
		// effect->explosionDuration = potionData.duration * multiplier;
		// effect->ingredientCount = potionData.mainEffectCount;
		
		return effect;
	}

	void SetThrowablePotionEffect(
		ThrowableObject* throwable,
		const Crafting::CraftedPotionData& potionData
	){
		throwable->SetEffectFactory(
			[potionData](SceneNode* node) -> EffectBase* {
				EffectBase* primaryEffect = AddPotionEffectToNode(
					node,
					potionData.primaryEffectId,
					potionData,
					false
				);

				if (potionData.HasSecondaryEffect()) {
					AddPotionEffectToNode(
						node,
						potionData.secondaryEffectId,
						potionData,
						true
					);
				}

				return primaryEffect;
			}
		);
	}

	SceneNode* FindNodeRecursive(SceneNode* node, const std::string& name) {
		if (node == nullptr) {
			return nullptr;
		}

		if (node->GetName() == name) {
			return node;
		}

		for (SceneNode* child : node->GetChildren()) {
			if (SceneNode* found = FindNodeRecursive(child, name)) {
				return found;
			}
		}

		return nullptr;
	}

	SceneNode* FindFirstPath(SceneNode* root, const std::vector<fs::path>& paths) {
		if (root == nullptr) {
			return nullptr;
		}

		for (const fs::path& path : paths) {
			if (SceneNode* found = root->FindNode(path)) {
				return found;
			}
		}

		return nullptr;
	}

	SceneNode* FindFirstNameRecursive(SceneNode* root, const std::vector<std::string>& names) {
		for (const std::string& name : names) {
			if (SceneNode* found = FindNodeRecursive(root, name)) {
				return found;
			}
		}

		return nullptr;
	}

	AnimationComponent::Animation* FindAnimation(
		AnimationComponent* animator,
		const std::string& name
	) {
		if (animator == nullptr || name.empty()) {
			return nullptr;
		}

		for (auto& animation : animator->animations) {
			if (animation.data.name == name) {
				return &animation;
			}
		}

		return nullptr;
	}

	std::string FindFirstAnimationName(
		AnimationComponent* animator,
		const std::vector<std::string>& names
	) {
		if (animator == nullptr) {
			return {};
		}

		for (const std::string& name : names) {
			if (FindAnimation(animator, name) != nullptr) {
				return name;
			}
		}

		return {};
	}
}

glm::vec3 PlayerController::GetMousePointOnGround(Camera* camera) {
	glm::vec2 mousePos = GetScene()->Input()->GetMousePosition();
	glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

	if (screenSize.x <= 0.0f || screenSize.y <= 0.0f) {
		return GlobalTransform().Position().Value();
	}

	float x = (2.0f * mousePos.x) / screenSize.x - 1.0f;
	float y = 1.0f - (2.0f * mousePos.y) / screenSize.y;

	glm::vec4 rayStartNdc(x, y, -1.0f, 1.0f);
	glm::vec4 rayEndNdc(x, y, 1.0f, 1.0f);

	glm::mat4 invVP = glm::inverse(camera->ProjectionMatrix() * camera->ViewMatrix());

	glm::vec4 rayStartWorld = invVP * rayStartNdc;
	glm::vec4 rayEndWorld = invVP * rayEndNdc;

	rayStartWorld /= rayStartWorld.w;
	rayEndWorld /= rayEndWorld.w;

	glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
	glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

	if (std::abs(rayDir.y) < 0.0001f) {
		return GlobalTransform().Position().Value();
	}

	float t = -rayOrigin.y / rayDir.y;
	return rayOrigin + rayDir * t;
}

glm::vec3 PlayerController::GetStrengthFromVelocity() {
	return this->charController->GetLinearVelocity() * this->velocityThrowBoost;
}

void PlayerController::Awake() {
	this->charController = GetObject<Physics::VirtualCharacterController>();
	this->aim = GetNode()->FindNode("Aim Reticle");

	assert(this->charController);

	SetupCharacterNodes();
	SetupAnimations();

	this->charController->SetCollisionLayerAndMask({1}, {0});

	if (this->throwingArm != nullptr) {
		this->defaultThrowingArmRotation =
			this->throwingArm->LocalTransform().Rotation();
	}

	this->instance = this;
}

void PlayerController::SetupCharacterNodes() {
	this->characterRoot = FindFirstPath(
		GetNode(),
		{
			"Bimberman/root",
			"Bimberman"
		}
	);

	if (this->characterRoot == nullptr) {
		this->characterRoot = GetNode();
		spdlog::warn(
			"PlayerController: Bimberman model root not found. Using Player node as fallback."
		);
	}

	this->torso = FindFirstPath(
		GetNode(),
		{
			"Bimberman/root/Torso",
			"Bimberman/rig_deform/DEF-spine/DEF-spine.001/DEF-spine.002/DEF-spine.003",
			"Bimberman/rig/MCH-torso.parent/torso"
		}
	);

	if (this->torso == nullptr) {
		this->torso = FindFirstNameRecursive(
			this->characterRoot,
			{
				"Torso",
				"torso",
				"DEF-spine.003",
				"DEF-spine.002",
				"DEF-spine.001",
				"DEF-spine"
			}
		);
	}

	if (this->torso == nullptr) {
		this->torso = this->characterRoot;
		spdlog::warn(
			"PlayerController: torso node not found. Body wobble will use model root."
		);
	}

	this->throwingArm = nullptr;
	if (this->torso != nullptr) {
		this->throwingArm = this->torso->FindNode("Arm R");
	}

	if (this->throwingArm == nullptr) {
		this->throwingArm = FindFirstPath(
			GetNode(),
			{
				"Bimberman/rig_deform/DEF-upper_arm.R",
				"Bimberman/rig/MCH-torso.parent/torso/MCH-spine.002/spine_fk.002/MCH-spine.003/spine_fk.003/tweak_spine.003/ORG-spine.003/ORG-shoulder.R/DEF-upper_arm.R"
			}
		);
	}

	if (this->throwingArm == nullptr) {
		this->throwingArm = FindFirstNameRecursive(
			this->characterRoot,
			{
				"Arm R",
				"DEF-upper_arm.R",
				"upper_arm.R",
				"upper_arm_fk.R"
			}
		);
	}

	this->rightHand = FindFirstPath(
		GetNode(),
		{
			"Bimberman/rig_deform/DEF-upper_arm.R/DEF-upper_arm.R.001/DEF-forearm.R/DEF-forearm.R.001/DEF-hand.R",
			"Bimberman/rig/MCH-torso.parent/torso/MCH-spine.002/spine_fk.002/MCH-spine.003/spine_fk.003/tweak_spine.003/ORG-spine.003/ORG-shoulder.R/DEF-upper_arm.R/DEF-upper_arm.R.001/DEF-forearm.R/DEF-forearm.R.001/DEF-hand.R"
		}
	);

	if (this->rightHand == nullptr) {
		this->rightHand = FindFirstNameRecursive(
			this->characterRoot,
			{
				"Hand R",
				"DEF-hand.R",
				"hand.R",
				"hand_fk.R"
			}
		);
	}

	this->throwPoint = nullptr;
	if (this->throwingArm != nullptr) {
		this->throwPoint = this->throwingArm->FindNode("Throw Point");
	}

	if (this->throwPoint == nullptr) {
		this->throwPoint = FindNodeRecursive(this->characterRoot, "Throw Point");
	}

	if (this->throwPoint == nullptr) {
		SceneNode* throwPointParent =
			this->rightHand != nullptr ? this->rightHand : this->throwingArm;

		if (throwPointParent != nullptr) {
			this->throwPoint = GetScene()->CreateNode(throwPointParent, "Throw Point");
			this->throwPoint->LocalTransform().Position() =
				glm::vec3(0.0f, 0.08f, 0.12f);
		}
	}

	if (this->throwingArm == nullptr) {
		spdlog::warn(
			"PlayerController: right arm node not found. Manual throw pose will be disabled."
		);
	}

	if (this->throwPoint == nullptr) {
		this->throwPoint = this->characterRoot;
		spdlog::warn(
			"PlayerController: Throw Point not found. Bottles will spawn from model root fallback."
		);
	}
}

void PlayerController::SetupAnimations() {
	this->animator = GetNode()->GetObjectInChildren<AnimationComponent>();

	if (this->animator == nullptr) {
		return;
	}

	for (auto& animation : this->animator->animations) {
		animation.playing = false;
		animation.looping = false;
	}

	this->walkAnimationName = FindFirstAnimationName(
		this->animator,
		{
			"run.001",
			"Run.001",
			"walk.001",
			"Walk.001",
			"run",
			"Run",
			"walk",
			"Walk"
		}
	);

	this->throwAnimationName = FindFirstAnimationName(
		this->animator,
		{
			"throw.001",
			"Throw.001",
			"throw",
			"Throw"
		}
	);

	if (this->walkAnimationName.empty()) {
		spdlog::warn("PlayerController: walk/run animation not found on player model.");
	}

	if (this->throwAnimationName.empty()) {
		spdlog::warn("PlayerController: throw animation not found on player model.");
	}
}

void PlayerController::StopAnimation(
	const std::string& animationName,
	bool resetToStart
) {
	AnimationComponent::Animation* animation =
		FindAnimation(this->animator, animationName);

	if (animation == nullptr) {
		return;
	}

	animation->playing = false;
	animation->looping = false;

	if (resetToStart) {
		this->animator->SetTime(animationName, 0.0f);
	}
}

void PlayerController::PlayLoopAnimation(const std::string& animationName) {
	if (this->animator == nullptr || animationName.empty()) {
		return;
	}

	if (this->activeLoopAnimationName == animationName) {
		return;
	}

	for (auto& animation : this->animator->animations) {
		animation.playing = false;
		animation.looping = false;
	}

	AnimationComponent::Animation* animation =
		FindAnimation(this->animator, animationName);

	if (animation == nullptr) {
		return;
	}

	animation->looping = true;
	animation->speed = 1.0f;
	this->animator->Play(animationName);
	this->activeLoopAnimationName = animationName;
}

void PlayerController::StopLoopAnimation() {
	if (this->activeLoopAnimationName.empty()) {
		return;
	}

	StopAnimation(this->activeLoopAnimationName, true);
	this->activeLoopAnimationName.clear();
}

void PlayerController::PlayThrowAnimation() {
	if (this->animator == nullptr || this->throwAnimationName.empty()) {
		return;
	}

	for (auto& animation : this->animator->animations) {
		animation.playing = false;
		animation.looping = false;
	}

	AnimationComponent::Animation* animation =
		FindAnimation(this->animator, this->throwAnimationName);

	if (animation == nullptr) {
		return;
	}

	animation->looping = false;
	animation->speed = 1.0f;
	this->animator->Play(this->throwAnimationName);

	this->throwAnimationActive = true;
	this->throwAnimationTimer = 0.0f;
	this->throwAnimationDuration = animation->data.duration;
	this->activeLoopAnimationName.clear();
}

void PlayerController::UpdateMovement() {
	Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
	if (!camera) return;

	glm::vec3 forward = camera->GlobalTransform().Forward();
	forward.y = 0.0f;

	if (glm::length(forward) < 0.001f) {
		forward = camera->GlobalTransform().Up();
		forward.y = 0.0f;
	}
	forward = glm::normalize(forward);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

	this->prevPositions.push(glm::vec4(GlobalTransform().Position().Value(), Time::Current()));

	glm::vec3 movement{0};

	if (GetScene()->Input()->KeyPressed(Key::W)) {
		movement += forward;
	}
	if (GetScene()->Input()->KeyPressed(Key::S)) {
		movement -= forward;
	}
	if (GetScene()->Input()->KeyPressed(Key::A)) {
		movement -= right;
	}
	if (GetScene()->Input()->KeyPressed(Key::D)) {
		movement += right;
	}

	float movementLength = glm::clamp(glm::length(movement), 0.f, 1.f);
	this->movementAmount = movementLength;

	if (movementLength > 0) {
		movement = glm::normalize(movement) * movementLength * this->speed;

		movement = glm::angleAxis(glm::sin(Time::Current() * this->woblinessFrequency) * (0.1f + this->wobliness * 0.4f), glm::vec3(0, 1, 0)) * movement;
	}
	else {
		movement = glm::vec3(0);
	}

	this->charController->Move(movement, Time::Delta());

	glm::vec3 posTimeSecondsAgo = this->prevPositions.front();

	while (this->prevPositions.front().w < Time::Current() - this->bodyDragTime) {
		posTimeSecondsAgo = this->prevPositions.front();

		this->prevPositions.pop();
	}

	posTimeSecondsAgo += glm::vec3(0, 4, 0);

	glm::quat fromTo = glm::identity<glm::quat>();

	glm::vec3 bodyDragDirection = posTimeSecondsAgo - GlobalTransform().Position();

	if (this->torso != nullptr && this->torso->GetParent() != nullptr) {
		bodyDragDirection =
			glm::inverse(this->torso->GetParent()->GlobalTransform().Value())
			* glm::vec4(bodyDragDirection, 0);

		glm::vec3 a = glm::cross(glm::vec3(0, 1, 0), bodyDragDirection);

		if (glm::length(a) > glm::epsilon<float>()) {
			fromTo.x = a.x;
			fromTo.y = a.y;
			fromTo.z = a.z;
			fromTo.w = glm::abs(glm::length(bodyDragDirection))
				+ glm::dot(bodyDragDirection, glm::vec3(0, 1, 0));

			fromTo = glm::normalize(fromTo);
		}

		this->torso->LocalTransform().Rotation() = fromTo;
	}
}

void PlayerController::UpdateTargetting() {
	Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
	if (!camera) return;

	glm::vec3 forward = camera->GlobalTransform().Forward();
	forward.y = 0.0f;

	if (glm::length(forward) < glm::epsilon<float>()) {
		forward = camera->GlobalTransform().Up();
		forward.y = 0.0f;
	}
	forward = glm::normalize(forward);
	glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

	glm::vec3 aimDir = GetMousePointOnGround(camera) - GlobalTransform().Position();

	float targetBearing = glm::atan(aimDir.x, aimDir.z);

	this->aimBearing = Math::MoveTowardsAngle(this->aimBearing, targetBearing, Time::Delta() * this->aimSpeed);

	if (this->characterRoot != nullptr) {
		this->characterRoot->LocalTransform().Rotation() =
			glm::angleAxis(this->aimBearing, glm::vec3(0, 1, 0));
	}

	if (!this->CanThrow()) {
		if (this->aim) {
			this->aim->SetEnabled(false);
		}

		return;
	}

	if (this->aim != nullptr) {
		this->aim->LocalTransform().Position() =
			glm::angleAxis(this->aimBearing, glm::vec3(0, 1, 0))
			* glm::vec3(0, 0, 1);
		this->aim->SetEnabled(GetScene()->Input()->ButtonPressed(0));
	}
}

void PlayerController::UpdateThrowing() {
	if (!this->CanThrow()) {
		return;
	}

	float throwOomph = 0;

	if (GetScene()->Input()->ButtonPressed(0)) {
		throwOomph = 1;
	}

	glm::vec3 aimDirection = glm::angleAxis(this->aimBearing, glm::vec3(0, 1, 0)) * glm::vec3(0, 0, 1);

	if (throwOomph > 0 && this->throwStrengthCache == 0) {
		this->throwStrengthAccum += Time::Delta() * throwOomph / this->throwSpeedTime;

		if (this->throwStrengthAccum > 1) {
			this->throwStrengthAccum = 1;
		}

		glm::vec3 hitPoint = aimDirection * glm::mix(this->minThrowDistance, this->maxThrowDistance, ThrowStrengthEasing(this->throwStrengthAccum)) + GetStrengthFromVelocity();

		this->aim->GlobalTransform().Position() = GlobalTransform().Position() + hitPoint;

		if (this->torso != nullptr) {
			this->torso->LocalTransform().Rotation() *= glm::angleAxis(
				ThrowStrengthEasing(this->throwStrengthAccum) * -0.2f,
				glm::vec3(1, 0, 0)
			);
		}

		if (this->throwingArm != nullptr) {
			this->throwingArm->LocalTransform().Rotation() = glm::angleAxis(
				glm::radians(-(160 + 60 * ThrowStrengthEasing(this->throwStrengthAccum))),
				glm::vec3(1, 0, 0)
			) * this->defaultThrowingArmRotation;
		}
	}
	else if (this->throwStrengthAccum > 0) {
		if (this->throwStrengthCache == 0) {
			this->throwStrengthCache = this->throwStrengthAccum;
		}

		this->throwStrengthAccum = Math::MoveTowards(this->throwStrengthAccum, 0, Time::Delta() * 10);

		if (this->throwStrengthCache > 0 && this->throwStrengthAccum < 0.7f) {
			Crafting::CraftedPotionData consumedPotionData;

			if (!PotionInventory::ConsumePotion(&consumedPotionData)) {
				this->throwStrengthCache = -1;
				return;
			}

			PlayThrowAnimation();

			SceneNode* thrownBottle = GetScene()->GetComponent<ThrowableObjectPool>()->RequestThrowableObject();
			auto* throwable = thrownBottle->AddObject<ThrowableObject>();

			float forwardVelocityBoost = glm::dot(GetStrengthFromVelocity(), aimDirection);
			if (forwardVelocityBoost < 0.0f) {
				forwardVelocityBoost = 0.0f;
			}

			float finalThrowDist = glm::mix(minThrowDistance, maxThrowDistance, ThrowStrengthEasing(this->throwStrengthCache));
			glm::vec3 hitPoint = aimDirection * (finalThrowDist + forwardVelocityBoost);

			float speedX = glm::length(hitPoint) / this->flightTime;

			float speedY = ((9.8 / 2) * this->flightTime * this->flightTime - glm::dot(this->throwPoint->GlobalTransform().Position().Value(), glm::vec3(0, 1, 0))) / this->flightTime;

			glm::vec3 throwDirection = GlobalTransform().Position() + hitPoint - this->throwPoint->GlobalTransform().Position();
			throwDirection.y = 0;

			glm::vec3 throwForce = glm::normalize(throwDirection) * speedX + glm::vec3(0, 1, 0) * speedY;

			thrownBottle->GetObject<Physics::Body>()->SetPosition(this->throwPoint->GlobalTransform().Position());

			thrownBottle->SetEnabled(true);
			SetThrowablePotionEffect(throwable,consumedPotionData);
			thrownBottle->GetObject<Physics::Body>()->SetLinearVelocity(throwForce);

			if (!PotionInventory::HasPotion()) {
				SetThrowingUnlocked(false);
			}

			this->throwStrengthCache = -1;
		}

		if (this->torso != nullptr) {
			this->torso->LocalTransform().Rotation() *= glm::angleAxis(
				ThrowStrengthEasing(this->throwStrengthAccum) * -0.2f,
				glm::vec3(1, 0, 0)
			);
		}

		if (this->throwingArm != nullptr) {
			this->throwingArm->LocalTransform().Rotation() = glm::angleAxis(
				glm::radians(-(220 * ThrowStrengthEasing(this->throwStrengthAccum))),
				glm::vec3(1, 0, 0)
			) * this->defaultThrowingArmRotation;
		}
	}
	else {
		this->throwStrengthCache = 0;

		if (this->throwingArm != nullptr) {
			this->throwingArm->LocalTransform().Rotation() =
				this->defaultThrowingArmRotation;
		}
	}
}

void PlayerController::UpdatePlayerAnimation() {
	if (this->animator == nullptr) {
		return;
	}

	if (this->throwAnimationActive) {
		this->throwAnimationTimer += Time::Delta();

		if (this->throwAnimationTimer >= this->throwAnimationDuration) {
			StopAnimation(this->throwAnimationName, true);
			this->throwAnimationActive = false;
		}
	}

	if (this->throwAnimationActive) {
		return;
	}

	if (this->movementAmount > 0.05f) {
		PlayLoopAnimation(this->walkAnimationName);
	} else {
		StopLoopAnimation();
	}
}

void PlayerController::HandleItemInteractions() {
	if (!this->pickableItemSystem) {
		this->pickableItemSystem = GetScene()->GetComponent<PickableItemSystem>();
		if (!this->pickableItemSystem) return;
	}

	PickableItem* newItem = nullptr;

	// Mouse cursor raycast
	if (!this->physics) {
		this->physics = this->GetScene()->GetComponent<Physics::System>();
	} else if (auto* camera = this->GetScene()->GetGraphics()->GetMainCamera()) {
		auto* input = this->GetScene()->Input();
		auto* graphics = this->GetScene()->GetGraphics();

		glm::vec2 mousePosition = input->GetMousePosition();
		glm::vec2 screenSize = graphics->GetScreenResolution();

		glm::vec4 viewport(0.0f, 0.0f, screenSize.x, screenSize.y);

		float windowY = screenSize.y - mousePosition.y;

		glm::vec3 windowNear(mousePosition.x, windowY, 0.0f);
		glm::vec3 windowFar(mousePosition.x, windowY, 1.0f);

		glm::mat4 view = camera->ViewMatrix();
		glm::mat4 proj = camera->ProjectionMatrix();

		glm::vec3 rayOrigin = glm::unProject(windowNear, view, proj, viewport);
		glm::vec3 rayTarget = glm::unProject(windowFar, view, proj, viewport);

		glm::vec3 rayDirection = glm::normalize(rayTarget - rayOrigin) * 100.0f;

		// Includes only the items (layer 2)
		Physics::LayerMaskFilter layerFilter({2}, true);

		Physics::RayCastPayload hit = this->physics->CastRay(rayOrigin, rayDirection, {}, {}, layerFilter);

		if (hit.hasHit && hit.node) {
			newItem = hit.node->GetObject<PickableItem>();
		}
	}

	// Closest item fallback
	if (newItem == nullptr) {
		newItem = pickableItemSystem->GetClosestItem(this->GlobalTransform().Position().Value(), this->itemHighlightRadius);
	}

	// spdlog::info("{}", newItem ? newItem->GetName() : "Null");

	// Highlighting logic
	if (newItem != this->highlightedItem) {
		if (this->highlightedItem) {
			for (auto* renderer : this->highlightedItem->GetNode()->GetAllObjectsInChildren<MeshRenderer>()) {
				renderer->maskFlags &= ~MaskEffectBits::Jfa;
			}
		}
		if (newItem != nullptr) {
			for (auto* renderer : newItem->GetNode()->GetAllObjectsInChildren<MeshRenderer>()) {
				renderer->maskFlags |= MaskEffectBits::Jfa;
			}
		}
		this->highlightedItem = newItem;
	}

	// On interact
	if (this->GetScene()->Input()->KeyDown(Key::F) && this->highlightedItem != nullptr) {
		this->highlightedItem->OnPickUp();
		delete this->highlightedItem->GetNode();
		this->highlightedItem = nullptr;
	}
}
void PlayerController::Update() {
	UpdateMovement();
	UpdateTargetting();
	UpdateThrowing();
	HandleItemInteractions();
	UpdatePlayerAnimation();

	if (this->torso != nullptr) {
		this->torso->GlobalTransform().Rotation() *= glm::angleAxis(
			glm::sin(Time::Current() * this->woblinessFrequency)
				* (0.1f + this->wobliness * 0.3f),
			this->torso->GlobalTransform().Forward()
		);
	}
}

void PlayerController::OnEnable() {
	PlayerController::instance = this;
}
void PlayerController::OnDisable() {
	if (PlayerController::instance == this) {
		PlayerController::instance = nullptr;
	}
}
	
void PlayerController::TakeDamage(float damage) {
	this->health -= damage;

	if (this->health < 0) {
		Die();
	}
}

float PlayerController::GetHealth() const {
	return this->health;
}
void PlayerController::SetHealth(float newHealth) {
	this->health = newHealth;
}

void PlayerController::Die() {
	spdlog::info("Player is dead!");

	delete this;
}

bool PlayerController::CanThrow() const {
	return this->throwingUnlocked && this->aim != nullptr && PotionInventory::HasPotion();
}

void PlayerController::DrawImGui() {
	ImGui::SliderFloat("Wobliness", &this->wobliness, 0, 1);
	ImGui::InputFloat("Wobliness Frequency", &this->woblinessFrequency);
	ImGui::InputFloat("Speed", &this->speed);
	ImGui::InputFloat("Body Drag Time", &this->bodyDragTime);
	ImGui::InputFloat("Aim Speed", &this->aimSpeed);
}
