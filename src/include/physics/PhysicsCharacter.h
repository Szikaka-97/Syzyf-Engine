#pragma once

#include "GameObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Character/Character.h>

class PhysicsCharacter : public GameObject {
public:
  float maxSeparationDistance = 0.1f;
private:
  JPH::Character* character = nullptr;
  JPH::Ref<JPH::CharacterSettings> characterSettings;
public:
  PhysicsCharacter();
  virtual ~PhysicsCharacter();

  // Getters
  JPH::Character* GetCharacter() const;

  glm::vec3 GetLinearVelocity() const;
  glm::vec3 GetPosition() const;
  glm::quat GetRotation() const;

  bool IsSupported() const;
  glm::vec3 GetGroundNormal() const;
  JPH::CharacterBase::EGroundState GetGroundState() const;

  // Setters
  void SetLinearVelocity(const glm::vec3& velocity);
  void AddLinearVelocity(const glm::vec3& velocity);
  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);

  void Awake();
  void OnEnable();
  void OnDisable();
};

