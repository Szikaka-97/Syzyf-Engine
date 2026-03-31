#pragma once

#include "Debug.h"
#include "GameObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Character/Character.h>

namespace Physics {
class CharacterController : public GameObject, public ImGuiDrawable {
public:
  float maxSeparationDistance = 1.0e-4f;
private:
  uint32_t collisionLayer = 1;
  uint32_t collisionMask = 1;
  
  JPH::Character* character = nullptr;
  JPH::Ref<JPH::CharacterSettings> characterSettings;
public:
  CharacterController(const JPH::Ref<JPH::CharacterSettings>& settings);
  virtual ~CharacterController();

  // Getters
  JPH::BodyID GetBodyID() const;

  uint32_t GetCollisionLayer() const;
  uint32_t GetCollisionMask() const;

  JPH::Character* GetCharacter() const;

  glm::vec3 GetLinearVelocity() const;
  glm::vec3 GetPosition() const;
  glm::quat GetRotation() const;
  float GetGravityFactor() const;

  bool IsSupported() const;

  JPH::BodyID GetGroundBodyID() const;
  SceneNode* GetGroundObject() const;
  glm::vec3 GetGroundPosition() const;
  glm::vec3 GetGroundNormal() const;
  glm::vec3 GetGroundVelocity() const;
  JPH::CharacterBase::EGroundState GetGroundState() const;

  // Setters
  void SetCollisionLayerAndMask(uint32_t layer, uint32_t mask);
  void SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, uint32_t mask = 0xFFFFFFFF);
  void SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, std::initializer_list<uint32_t> collideWithLayers);

  void AddImpulse(const glm::vec3& impulse);

  void SetLinearVelocity(const glm::vec3& velocity);
  void AddLinearVelocity(const glm::vec3& velocity);
  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);
  void SetGravityFactor(float factor);

  void SetUp(const glm::vec3& up);
  bool SetShape(const JPH::RefConst<JPH::Shape>& shape, float maxPenetrationDepth = 1.0e-4f);

  void Awake();
  void OnEnable();
  void OnDisable();

  void DrawImGui();
};
}
