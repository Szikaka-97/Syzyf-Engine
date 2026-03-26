#pragma once

#include "Debug.h"
#include "GameObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <glm/fwd.hpp>
namespace Physics {
class VirtualCharacterController : public GameObject, public ImGuiDrawable {
private:
  uint32_t collisionLayer = 1;
  float gravityFactor = 1.0f;

  JPH::Ref<JPH::CharacterVirtual> character;
  JPH::Ref<JPH::CharacterVirtualSettings> characterSettings;

public:
  VirtualCharacterController(const JPH::Ref<JPH::CharacterVirtualSettings>& settings);
  virtual ~VirtualCharacterController();

  void Move(const glm::vec3& velocity, float deltaTime);

  glm::vec3 GetPosition() const;
  glm::quat GetRotation() const;
  float GetGravityFactor() const;
  glm::vec3 GetLinearVelocity() const;

  JPH::BodyID GetGroundBodyID() const;
  SceneNode* GetGroundObject() const;
  glm::vec3 GetGroundPosition() const;
  glm::vec3 GetGroundNormal() const;
  glm::vec3 GetGroundVelocity() const;
  JPH::CharacterBase::EGroundState GetGroundState() const;
  bool IsSupported() const;

  void SetCollisionLayer(uint32_t layer);
  void SetCollisionLayer(std::initializer_list<uint32_t> layers);

  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);
  void SetGravityFactor(float factor);

  void Awake();

  void DrawImGui();
};
}
