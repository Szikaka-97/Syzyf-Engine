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

JPH::Character* PhysicsCharacter::GetCharacter() const {
  return this->character;
}

glm::vec3 PhysicsCharacter::GetLinearVelocity() const {
  if (this->character) {
    JPH::Vec3 velocity = this->character->GetLinearVelocity();
    return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
  }
  return glm::vec3(0.0f);
}

glm::vec3 PhysicsCharacter::GetPosition() const {
  if (this->character) {
    JPH::RVec3 position = this->character->GetPosition();
    return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
  }
  return glm::vec3(0.0f);
}

glm::quat PhysicsCharacter::GetRotation() const {
  if (this->character) {
    JPH::Quat rotation = this->character->GetRotation();
    return glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
  }
  return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

glm::vec3 PhysicsCharacter::GetGroundNormal() const {
  if (this->character) {
    JPH::Vec3 normal = this->character->GetGroundNormal();
    return glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
  }
  return glm::vec3(0.0f, 1.0f, 0.0f);
}

JPH::CharacterBase::EGroundState PhysicsCharacter::GetGroundState() const {
  if (this->character) {
    return this->character->GetGroundState();
  }
  return JPH::CharacterBase::EGroundState::InAir;
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
