#pragma once

#include "Debug.h"
#include "GameObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Character/Character.h>

namespace Physics {
class CharacterController : public GameObject, public ImGuiDrawable {
public:
  float maxSeparationDistance = 0.1f;

private:
  uint32_t collisionLayer = 1;
  uint32_t collisionMask = 1;
  
  JPH::Character* character = nullptr;
  JPH::Ref<JPH::CharacterSettings> characterSettings;
public:
  CharacterController();
  virtual ~CharacterController();

  // Getters
  uint32_t GetCollisionLayer() const;
  uint32_t GetCollisionMask() const;
  JPH::Character* GetCharacter() const;

  glm::vec3 GetLinearVelocity() const;
  glm::vec3 GetPosition() const;
  glm::quat GetRotation() const;

  bool IsSupported() const;
  glm::vec3 GetGroundNormal() const;
  JPH::CharacterBase::EGroundState GetGroundState() const;

  // Setters
  void SetCollisionLayerAndMask(uint32_t layer, uint32_t mask);
  
  void SetLinearVelocity(const glm::vec3& velocity);
  void AddLinearVelocity(const glm::vec3& velocity);
  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);

  void Awake();
  void OnEnable();
  void OnDisable();

  void DrawImGui();
};
}
