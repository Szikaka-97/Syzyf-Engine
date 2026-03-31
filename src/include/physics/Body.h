#pragma once

#include "Debug.h"
#include "Mesh.h"
#include "GameObject.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <glm/fwd.hpp>
#include <initializer_list>
#include <spdlog/spdlog.h>

namespace Physics {
class Body : public GameObject, public ImGuiDrawable {
private:
  static constexpr float defaultConvexRadius = 0.05f;

  JPH::BodyID bodyID;
  JPH::BodyCreationSettings bodyCreationSettings;

  uint32_t collisionLayer = 1;
  uint32_t collisionMask = 1;

  bool bodyCreated = false;
  bool addedToWorld = false;
public:
  Body(const JPH::BodyCreationSettings& settings);

  static JPH::BodyCreationSettings Sphere(float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  static JPH::BodyCreationSettings Box(glm::vec3 halfExtent, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  static JPH::BodyCreationSettings Capsule(float halfHeight, float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  static JPH::BodyCreationSettings Plane(glm::vec3 normal, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  static JPH::BodyCreationSettings ConvexHullMesh(const Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer);
  static JPH::BodyCreationSettings Mesh(const Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer);

  virtual ~Body();

  // Getters
  JPH::BodyID GetBodyID() const;

  glm::vec3 GetPosition() const;
  glm::quat GetRotation() const;
  glm::vec3 GetLinearVelocity() const;
  glm::vec3 GetAngularVelocity() const;

  float GetFriction() const;
  float GetRestitution() const;
  float GetGravityFactor() const;
  float GetLinearDamping() const;
  float GetAngularDamping() const;

  JPH::EMotionType GetMotionType() const;
  bool IsActive() const;
  bool IsSensor() const;

  // Setters
  void SetCollisionLayerAndMask(uint32_t layer, uint32_t mask);
  void SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, uint32_t mask = 0xFFFFFFFF);
  void SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, std::initializer_list<uint32_t> collideWithLayers);
  
  void SetShape(const JPH::ShapeRefC shape); 

  void SetPosition(const glm::vec3& position);
  void SetRotation(const glm::quat& rotation);
  void SetLinearVelocity(const glm::vec3& velocity);
  void SetAngularVelocity(const glm::vec3& velocity);

  void SetFriction(const float friction);
  void SetRestitution(const float restitution);
  void SetGravityFactor(const float factor);
  void SetLinearDamping(const float damping);
  void SetAngularDamping(const float damping);

  void SetMotionType(const JPH::EMotionType motionType);
  void SetActivationState(const bool activation);

  void SetIsSensor(const bool isSensor);

  void ApplyForce(const glm::vec3& force);
  void ApplyImpulse(const glm::vec3& impulse);
  void ApplyTorque(const glm::vec3& torque);
  void ApplyAngularImpulse(const glm::vec3& impulse);

  void Awake();
  void OnEnable();
  void OnDisable();

  void DrawImGui();

  private:
  Body();
};
}
