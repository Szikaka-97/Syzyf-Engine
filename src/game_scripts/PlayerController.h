#pragma once

// glm::mix doesn't clamp the alpha operant. I don't know how it'll affect the game

#include "AimingAid.h"
#include <GameObject.h>
#include <Scene.h>
#include <InputSystem.h>
#include <Graphics.h>
#include <Camera.h>
#include <game_scripts/ThrowBottle.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <physics/VirtualCharacterController.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterBase.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <cmath>
#include "AimingAid.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "game_scripts/CameraSettings.h"
#include "physics/Body.h"
#include "physics/Helpers.h"
#include "physics/System.h"

#include "game_scripts/AttackEffects/EffectsManager.h"
#include "game_scripts/AttackEffects/BottleEffectDelivery.h"

class PlayerController : public GameObject, public ImGuiDrawable {
public:
	float wobbliness = 1;
	float speed = 8;
	float velocityThrowBoost = 1;
	float sidewaysWobbleFrequency = 0;
	glm::vec3 desiredMovement = glm::vec3(0.0f);

	SceneNode* throwingArm = nullptr;
	SceneNode* torso = nullptr;
	SceneNode* throwPoint = nullptr;

	glm::quat baseArmRotation = glm::quat(1, 0, 0, 0);
	glm::vec3 targetOffset = glm::vec3(0.0f);
	AimingAid* aim = nullptr;
	float throwSpeedTime = 0.6f;
	float minThrowDistance = 1;
	float maxThrowDistance = 5;
	float flightTime = 1;
	float throwStrengthAccum = 0;
	float throwStrengthCache = 0;

	float wobblinessAccum = 0;

	SceneNode* bottle = nullptr;
	EffectBase* currentEffect = nullptr;
	//MeshRenderer* bottleRenderer = nullptr;
	std::function<void(SceneNode*)> m_EffectFactory;

	Mesh*     bottleMesh     = nullptr;
Material* bottleMaterial = nullptr;



	Physics::VirtualCharacterController* virtualController = nullptr;
	glm::vec3 velocity = glm::vec3(0.0f);

	void SetEffect(EffectBase* effect) {
		this->currentEffect = effect;

	}
	void SetEffectFactory(std::function<void(SceneNode*)> factory) {
    m_EffectFactory = std::move(factory);
}
	void SetBottleResources(Mesh* mesh, Material* mat) {
    bottleMesh     = mesh;
    bottleMaterial = mat;
}

	glm::vec3 GetMousePointOnGround(Camera* camera) {
		glm::vec2 mousePos = GetScene()->Input()->GetMousePosition();
		glm::vec2 screenSize = GetScene()->GetGraphics()->GetScreenResolution();

		if (screenSize.x <= 0.0f || screenSize.y <= 0.0f) {
			return GlobalTransform().Position().Value();
		}

		// spdlog::info("{}, {} : {}, {}", mousePos.x, mousePos.y, screenSize.x, screenSize.y);


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

	void TryInitController() {
		if (!virtualController) {
			virtualController = GetObject<Physics::VirtualCharacterController>();
			virtualController->SetCollisionLayerAndMask({1}, {0});
		}
	}

	float ThrowStrengthEasing(float strength) {
		return -(glm::cos(glm::pi<float>() * strength) - 1) / 2;
	}

	glm::vec3 GetStrengthFromVelocity() {
		return this->virtualController->GetLinearVelocity() * velocityThrowBoost;
	}

	float MoveTowards(float current, float target, float maxDelta) {
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
public:
	PlayerController() {
		this->throwingArm = GetNode()->FindNode("Bimberman/root/Torso/Arm R");
		this->throwPoint = GetNode()->FindNode("Bimberman/root/Torso/Arm R/Throw Point");

		spdlog::warn(this->throwPoint != nullptr);

		this->aim->SetStretch(-1);

		this->baseArmRotation = this->throwingArm->LocalTransform().Rotation();

		this->throwStrengthAccum = 0;
		this->throwStrengthCache = 0;

		this->bottle = GltfImporter::LoadScene(GetScene(), "./res/models/crosshair.glb", "bottle visual");
	}

	PlayerController(SceneNode* markerNode) {
		this->throwingArm = GetNode()->FindNode("Bimberman/root/Torso/Arm R");
		this->throwPoint = GetNode()->FindNode("Bimberman/root/Torso/Arm R/Throw Point");

		spdlog::warn(this->throwPoint != nullptr);

		this->aim->SetStretch(-1);

		this->baseArmRotation = this->throwingArm->LocalTransform().Rotation();

		this->throwStrengthAccum = 0;
		this->throwStrengthCache = 0;

		this->bottle = GltfImporter::LoadScene(GetScene(), "./res/models/crosshair.glb", "bottle visual");
	}


	void Update() {
		TryInitController();
		if (!virtualController) return;

		Camera* camera = GetScene()->GetGraphics()->GetMainCamera();
		if (!camera) return;

        glm::vec3 forward = camera->GlobalTransform().Forward();
        forward.y = 0.0f;

        if (glm::length(forward) < 0.001f) {
            forward = camera->GlobalTransform().Up();
            forward.y = 0.0f;
        }

        forward = glm::normalize(forward);
        glm::vec3 left = glm::cross(forward, glm::vec3(0, 1, 0));

		glm::vec3 movement = glm::zero<glm::vec3>();

		if (GetScene()->Input()->KeyPressed(Key::W)) {
			movement += forward * speed;
		}
		if (GetScene()->Input()->KeyPressed(Key::S)) {
			movement -= forward * speed;
		}
		if (GetScene()->Input()->KeyPressed(Key::A)) {
			movement -= left * speed;
		}
		if (GetScene()->Input()->KeyPressed(Key::D)) {
			movement += left * speed;
		}

        float velocityLerpFactor = 0.0f;
        if (this->speed > 0.001f) {
            velocityLerpFactor = glm::dot(this->desiredMovement, movement) / (this->speed * this->speed);
        }
		velocityLerpFactor *= velocityLerpFactor;

		float minVelocityLerpFactor = glm::length(this->desiredMovement) < glm::length(movement) ? 0.03f : 0.3f;
		float maxVelocityLerpFactor = glm::length(this->desiredMovement) < glm::length(movement) ? 0.2f : 0.25f;

		this->desiredMovement = glm::mix(this->desiredMovement, movement, glm::mix(minVelocityLerpFactor, maxVelocityLerpFactor, velocityLerpFactor));

		this->virtualController->Move(this->desiredMovement, Time::Delta());
		
		glm::vec3 targetPos = GetMousePointOnGround(camera);

		this->targetOffset = targetPos - GlobalTransform().Position();
		// this->aim->PointAt(targetPos);
		this->aim->SetCrosshairPosition(targetPos);

        if (glm::length(this->targetOffset) > 0.001f) { 
		    GlobalTransform().Rotation() = glm::quatLookAt(glm::normalize(-targetOffset), glm::vec3(0, 1, 0));
        }

		float throwOomph = 0;

		if (GetScene()->Input()->ButtonPressed(0)) {
			throwOomph = 1;
		}
		// else if (Input.GetAxis("Throw Thing Gamepad") > 0)
		// {
		// 	throwOomph = Input.GetAxis("Throw Thing Gamepad");
		// }

		if (throwOomph > 0 && this->throwStrengthCache == 0) {
			this->throwStrengthAccum += Time::Delta() * throwOomph / this->throwSpeedTime;

			if (this->throwStrengthAccum > 1) {
				this->throwStrengthAccum = 1;
			}

			this->throwingArm->LocalTransform().Rotation() = glm::angleAxis(glm::radians(-(160 + 60 * ThrowStrengthEasing(this->throwStrengthAccum))), glm::vec3(1, 0, 0)) * baseArmRotation;

			this->aim->SetStretch(ThrowStrengthEasing(this->throwStrengthAccum));

            glm::vec3 targetDir = glm::length(this->targetOffset) > 0.001f ? glm::normalize(this->targetOffset) : GlobalTransform().Forward();

            glm::vec3 hitPoint = targetDir * glm::mix(minThrowDistance, maxThrowDistance, ThrowStrengthEasing(this->throwStrengthAccum)) + GetStrengthFromVelocity();

            this->aim->crosshair->SetEnabled(true);
            this->aim->SetCrosshairPosition(GlobalTransform().Position() + hitPoint);

			// this.torso.localRotation = Quaternion.AngleAxis(ThrowStrengthEasing(this.throwStrengthAccum) * -10, Vector3.right);
		}
		else if (this->throwStrengthAccum > 0) {
			if (this->throwStrengthCache == 0) {
				this->throwStrengthCache = this->throwStrengthAccum;
			}

			this->throwStrengthAccum = MoveTowards(this->throwStrengthAccum, 0, Time::Delta() * 10);

if (this->throwStrengthCache > 0 && this->throwStrengthAccum < 0.7f) {
    SceneNode* thrownBottle = GetScene()->CreateNode("Thrown Bottle");
    thrownBottle->GlobalTransform().Position() =
        throwPoint->GlobalTransform().Position().Value();

    thrownBottle->AddObject<Physics::Body>(
        JPH::BodyCreationSettings(
            Physics::SphereShape(0.1f),
            JPH::Vec3::sZero(),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Physics::Layers::MOVING));
    thrownBottle->GetObject<Physics::Body>()->SetCollisionLayerAndMask({0}, 0);

    // Visual jako TOP-LEVEL węzeł — NIE dziecko butelki
    SceneNode* visual = nullptr;
    if (bottleMesh && bottleMaterial) {
        visual = GetScene()->CreateNode("BottleVisual");
        // Pozycja synchronizowana przez BottleEffectDelivery::Update()
        visual->GlobalTransform().Position() =
            thrownBottle->GlobalTransform().Position().Value();
        visual->AddObject<MeshRenderer>(bottleMesh, bottleMaterial);
    }

    if (m_EffectFactory) {
        auto* delivery = thrownBottle->AddObject<BottleEffectDelivery>();
        delivery->SetEffectFactory(m_EffectFactory);
        delivery->SetVisualNode(visual);  // delivery zarządza jego życiem
    }


    //// Visual jako TOP-LEVEL węzeł — NIE dziecko butelki
    //SceneNode* visual = nullptr;
    //if (bottleMesh && bottleMaterial) {
    //    visual = GetScene()->CreateNode("BottleVisual");
    //    // Pozycja synchronizowana przez BottleEffectDelivery::Update()
    //    visual->GlobalTransform().Position() =
    //        thrownBottle->GlobalTransform().Position().Value();
    //    visual->AddObject<MeshRenderer>(bottleMesh, bottleMaterial);
    //}

    //if (m_EffectFactory) {
    //    auto* delivery = thrownBottle->AddObject<BottleEffectDelivery>();
    //    delivery->SetEffectFactory(m_EffectFactory);
    //    delivery->SetVisualNode(visual);  // delivery zarządza jego życiem
    //}

    // ... obliczenia prędkości bez zmian ...
    throwStrengthCache = -1;

    // ... reszta obliczeń prędkości bez zmian ...

				// Fill up thrownBottle

                glm::vec3 targetDir = glm::length(this->targetOffset) > 0.001f ? glm::normalize(this->targetOffset) : GlobalTransform().Forward();

				glm::vec3 hitPoint = targetDir * glm::mix(minThrowDistance, maxThrowDistance, ThrowStrengthEasing(this->throwStrengthCache)) + GetStrengthFromVelocity();

				float speedX = glm::length(hitPoint) / this->flightTime;

				float speedY = ((glm::dot(glm::vec3(0, -9.8, 0), glm::vec3(0, -1, 0)) / 2) * this->flightTime * this->flightTime - glm::dot(this->throwPoint->GlobalTransform().Position().Value(), glm::vec3(0, 1, 0))) / this->flightTime;

				glm::vec3 throwDirection = this->GlobalTransform().Position() + hitPoint - this->throwPoint->GlobalTransform().Position();
				throwDirection.y = 0;

                throwDirection = glm::length(throwDirection) > 0.001f ? glm::normalize(throwDirection) : GlobalTransform().Forward();

				glm::vec3 throwForce = glm::normalize(throwDirection) * speedX + glm::vec3(0, 1, 0) * speedY;

				// thrownBottle.GetComponent<Rigidbody>().AddForce(throwDirection, ForceMode.VelocityChange);
				thrownBottle->GetObject<Physics::Body>()->SetLinearVelocity(throwForce);

				this->throwStrengthCache = -1;
			}

			this->throwingArm->LocalTransform().Rotation() = glm::angleAxis(glm::radians(-(220 * ThrowStrengthEasing(this->throwStrengthAccum))), this->GlobalTransform().Right()) * baseArmRotation;

			this->aim->SetStretch(-1);

			this->aim->crosshair->SetEnabled(false);

			// this->torso->LocalTransform().Rotation() = glm::angleAxis(glm::radians(10 + ThrowStrengthEasing(this->throwStrengthAccum) * -20), glm::vec3(-1, 0, 0));
		}
		else {
			this->throwStrengthCache = 0;

			this->throwingArm->LocalTransform().Rotation() = baseArmRotation;

			// this->torso->LocalTransform().Rotation() = glm::quat();
		}

		// glm::vec3 movement(0.0f);
		// glm::vec3 cameraRight = camera->GlobalTransform().Right();
		// glm::vec3 cameraForward = camera->GlobalTransform().Forward();

		// cameraRight.y = 0.0f;
		// cameraForward.y = 0.0f;

		// if (glm::length(cameraRight) > 0.0f) {
		// 	cameraRight = glm::normalize(cameraRight);
		// }
		// if (glm::length(cameraForward) > 0.0f) {
		// 	cameraForward = glm::normalize(cameraForward);
		// }

		// if (GetScene()->Input()->KeyPressed(Key::W)) {
		// 	movement += cameraForward;
		// }
		// if (GetScene()->Input()->KeyPressed(Key::S)) {
		// 	movement -= cameraForward;
		// }
		// if (GetScene()->Input()->KeyPressed(Key::A)) {
		// 	movement += cameraRight;
		// }
		// if (GetScene()->Input()->KeyPressed(Key::D)) {
		// 	movement -= cameraRight;
		// }

		// if (glm::length(movement) > 0.0f) {
		// 	movement = glm::normalize(movement);
		// }

		// velocity.x = movement.x * moveSpeed;
		// velocity.z = movement.z * moveSpeed;

		// if (virtualController->IsSupported()) {
		// 	velocity.y = 0.0f;

		// 	if (GetScene()->Input()->KeyPressed(Key::Space) &&
		// 		virtualController->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround) {
		// 		velocity.y = jumpSpeed;
		// 	}
		// } else {
		// 	velocity.y += -9.81f * virtualController->GetGravityFactor() * (1.0f / 60.0f);
		// }

		// virtualController->Move(velocity, 1.0f / 60.0f);

		// GlobalTransform().Position() = virtualController->GetPosition();

		// glm::vec3 mouseWorld = GetMousePointOnGround(camera);

		// if (markerNode) {
		// 	glm::vec3 markerPos = mouseWorld;
		// 	markerPos.y += 0.02f;
		// 	markerNode->GlobalTransform().Position() = markerPos;
		// }

		// glm::vec3 toMouse = mouseWorld - GlobalTransform().Position().Value();
		// toMouse.y = 0.0f;

		// if (glm::length(toMouse) > 0.001f) {
		// 	toMouse = glm::normalize(toMouse);
		// 	float angle = std::atan2(toMouse.x, toMouse.z);
		// 	GlobalTransform().Rotation() = glm::angleAxis(angle, glm::vec3(0, 1, 0));
		// 	virtualController->SetRotation(GlobalTransform().Rotation().Value());
		// }

		// if (GetScene()->Input()->ButtonDown(MouseButton::Left)) {
		// 	if (bottleThrower) {
		// 		glm::vec3 startPos = GlobalTransform().Position().Value() + glm::vec3(0.0f, 1.2f, 0.0f);
		// 		bottleThrower->LaunchBottle(startPos, mouseWorld, 0.8f, 3.0f);
		// 	}
		// }
	}

	void DrawImGui() override {
		// ImGui::InputFloat("Player move speed", &moveSpeed);
		// ImGui::InputFloat("Jump speed", &jumpSpeed);
	}
};
