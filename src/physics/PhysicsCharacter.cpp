#include "physics/PhysicsCharacter.h"
#include "Jolt/Math/Math.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/EActivation.h"
#include "physics/PhysicsComponent.h"
#include <spdlog/spdlog.h>

PhysicsCharacter::PhysicsCharacter() {
  JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();

  settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
  settings->mLayer = PhysicsComponent::Layers::MOVING;
  settings->mShape = new JPH::SphereShape(0.75f);
  settings->mFriction = 5.0f;

  this->characterSettings = settings;
  
  spdlog::info("PhysicsCharacter: Added a character controller");
}

PhysicsCharacter::~PhysicsCharacter() {
  delete this->character;
}

void PhysicsCharacter::Awake() {
  PhysicsComponent* physics = this->GetScene()->GetComponent<PhysicsComponent>();
  if (physics == nullptr) {
    spdlog::warn("Tried waking up a physics character without a PhysicsComponent");
    return;
  }

  glm::vec3 nodePosition = this->GetTransform().GlobalTransform().Position();
  glm::quat nodeRotation = this->GetTransform().GlobalTransform().Rotation();

  JPH::RVec3 position = JPH::RVec3(nodePosition.x, nodePosition.y, nodePosition.z);
  JPH::Quat rotation = JPH::Quat(nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w);

  // ignoring scale

  this->character = new JPH::Character(this->characterSettings, position, rotation, 0, &physics->GetSystem());

  spdlog::info("PhysicsCharacter: A character controller called Awake()");
}

void PhysicsCharacter::OnEnable() {
  PhysicsComponent* physics = this->GetScene()->GetComponent<PhysicsComponent>();
  if (physics == nullptr) {
    spdlog::warn("Tried waking up a physics character without a PhysicsComponent");
    return;
  }

  if (this->character != nullptr) {
    this->character->AddToPhysicsSystem(JPH::EActivation::Activate);
    spdlog::info("PhysicsCharacter: Character controller activated");
  }
}

void PhysicsCharacter::OnDisable() {
  if (this->character != nullptr) {
    this->character->RemoveFromPhysicsSystem();
  }
}
