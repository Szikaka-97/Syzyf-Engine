#include "physics/Body.h"
#include "physics/System.h"

#include "GameObject.h"
#include "Jolt/Core/Core.h"
#include "Jolt/Geometry/Triangle.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "Jolt/Physics/Collision/Shape/PlaneShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/EActivation.h"
#include <spdlog/spdlog.h>
#include <imgui.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

namespace Physics {
using namespace JPH::literals;

Body::Body() {};

Body::Body(const JPH::BodyCreationSettings& settings): bodyCreationSettings(settings) {}

JPH::BodyCreationSettings Body::Sphere(float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  // If the radius is to small it complains about not being able to calculate the mass and does a SIGTRAP
  if (radius < 0.001f) {
    spdlog::warn("Trying to create a `PhysicsObjet::Sphere` with too small of a radius, setting it to 0.001");
    radius = 0.001f; 
  }

  return JPH::BodyCreationSettings(
  new JPH::SphereShape(radius),
    JPH::RVec3::sZero(),
    JPH::QuatArg::sIdentity(),
    type,
    layer
  );
}

JPH::BodyCreationSettings Body::Box(glm::vec3 halfExtent, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  if (halfExtent.x < defaultConvexRadius || halfExtent.y < defaultConvexRadius || halfExtent.z < defaultConvexRadius) {
    spdlog::warn("Trying to create a `PhysicsObject::Box` with extents smaller than Jolt's default convex radius. Clamping to 0.05f");
    halfExtent.x = std::max(halfExtent.x, defaultConvexRadius);
    halfExtent.y = std::max(halfExtent.y, defaultConvexRadius);
    halfExtent.z = std::max(halfExtent.z, defaultConvexRadius);
  }
  
  return JPH::BodyCreationSettings(
      new JPH::BoxShape(JPH::Vec3Arg(halfExtent.x, halfExtent.y, halfExtent.z)),
      JPH::RVec3Arg::sZero(),
      JPH::QuatArg::sIdentity(),
      type,
      layer
  );
}

JPH::BodyCreationSettings Body::Capsule(float halfHeight, float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  if (halfHeight < defaultConvexRadius || radius < defaultConvexRadius) {
    spdlog::warn("Trying to create a `PhysicsObject::Capsule` with dimensions smaller than Jolt's convex radius. Clamping to 0.05f");
    halfHeight = std::max(halfHeight, defaultConvexRadius);
    radius = std::max(halfHeight, defaultConvexRadius);
  }

  return JPH::BodyCreationSettings(
      new JPH::CapsuleShape(halfHeight, radius),
      JPH::RVec3Arg::sZero(),
      JPH::QuatArg::sIdentity(),
      type,
      layer
  );    
}

JPH::BodyCreationSettings Body::Plane(glm::vec3 normal, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  return JPH::BodyCreationSettings (
    new JPH::PlaneShape,
    JPH::RVec3Arg::sZero(),
    JPH::QuatArg::sIdentity(),
    type,
    layer
  );
}

JPH::BodyCreationSettings Body::ConvexHullMesh(const class Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  const uint8_t* vertexDataPointer = reinterpret_cast<const uint8_t*>(mesh->GetVertexData());
  const unsigned int vertexStride = mesh->GetVertexStride() * sizeof(float);
  const unsigned int vertexCount = mesh->GetVertexCount();

  std::vector<JPH::Vec3> joltVertices;
  joltVertices.reserve(vertexCount);

  for (unsigned int i = 0; i < mesh->GetVertexCount(); i++) {
    const float* pointer = reinterpret_cast<const float*>(vertexDataPointer);

    joltVertices.emplace_back(
      pointer[0],
      pointer[1],
      pointer[2]
    );

    vertexDataPointer += vertexStride;
  }

  JPH::ConvexHullShapeSettings* shapeSettings = new JPH::ConvexHullShapeSettings(
    joltVertices.data(),
    joltVertices.size()
  );

  return JPH::BodyCreationSettings(
    shapeSettings,
    JPH::RVec3Arg::sZero(),
    JPH::QuatArg::sIdentity(),
    type,
    layer
  );
}

JPH::BodyCreationSettings Body::Mesh(const class Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  const uint8_t* vertexDataPointer = reinterpret_cast<const uint8_t*>(mesh->GetVertexData());
  const unsigned int vertexStride = mesh->GetVertexStride() * sizeof(float);
  const unsigned int vertexCount = mesh->GetVertexCount();

  JPH::TriangleList triangles;
  triangles.reserve(vertexCount / 3);

  for (unsigned int i = 0; i < vertexCount; i += 3) {
    const float* p1 = reinterpret_cast<const float*>(vertexDataPointer);
    JPH::Vec3 v1(p1[0], p1[1], p1[2]);
    vertexDataPointer += vertexStride;

    const float* p2 = reinterpret_cast<const float*>(vertexDataPointer);
    JPH::Vec3 v2(p2[0], p2[1], p2[2]);
    vertexDataPointer += vertexStride;

    const float* p3 = reinterpret_cast<const float*>(vertexDataPointer);
    JPH::Vec3 v3(p3[0], p3[1], p3[2]);
    vertexDataPointer += vertexStride;

    triangles.emplace_back(v1, v2, v3);
  }

  JPH::MeshShapeSettings* shapeSettings = new JPH::MeshShapeSettings(triangles);

  shapeSettings->Sanitize();

  return JPH::BodyCreationSettings(
    shapeSettings,
    JPH::RVec3Arg::sZero(),
    JPH::QuatArg::sIdentity(),
    type,
    layer
  );
 
}

Body::~Body() {
  if (bodyCreated) {
    System* physics = GetScene()->GetComponent<System>();

    if (!physics) {
      spdlog::error("Failed to retrieve `PhysicsComponent` when trying to destruct `PhysicsObject` did it get destructed earlier?");
      return;
    }

    if (addedToWorld) {
      physics->GetBodyInterface().RemoveBody(bodyID);
    }
    physics->GetBodyInterface().DestroyBody(bodyID);
  }
}

JPH::BodyID Body::GetBodyID() const {
  return this->bodyID;
}

glm::vec3 Body::GetPosition() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::RVec3 position = physics->GetBodyInterface().GetPosition(bodyID);
      return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
    }
  }
  return glm::vec3(bodyCreationSettings.mPosition.GetX(), bodyCreationSettings.mPosition.GetY(), bodyCreationSettings.mPosition.GetZ());
}

glm::quat Body::GetRotation() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::Quat rotation = physics->GetBodyInterface().GetRotation(bodyID);
      return glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    }
  }
  return glm::quat(
    bodyCreationSettings.mRotation.GetW(),
    bodyCreationSettings.mRotation.GetX(),
    bodyCreationSettings.mRotation.GetY(),
    bodyCreationSettings.mRotation.GetZ()
  );
}

glm::vec3 Body::GetLinearVelocity() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::Vec3 velocity = physics->GetBodyInterface().GetLinearVelocity(bodyID);
      return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
    }
  }
    return glm::vec3(
      bodyCreationSettings.mLinearVelocity.GetX(),
      bodyCreationSettings.mLinearVelocity.GetY(),
      bodyCreationSettings.mLinearVelocity.GetZ()
    );
}

glm::vec3 Body::GetAngularVelocity() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::Vec3 velocity = physics->GetBodyInterface().GetAngularVelocity(bodyID);
      return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
    }
  }
    return glm::vec3(
      bodyCreationSettings.mAngularVelocity.GetX(),
      bodyCreationSettings.mAngularVelocity.GetY(),
      bodyCreationSettings.mAngularVelocity.GetZ()
    );
}

float Body::GetFriction() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().GetFriction(bodyID);
    }
  }
  return bodyCreationSettings.mFriction;
}

float Body::GetRestitution() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().GetRestitution(bodyID);
    }
  }
  return bodyCreationSettings.mRestitution;
}

float Body::GetGravityFactor() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().GetGravityFactor(bodyID);
    }
  }
  return bodyCreationSettings.mGravityFactor;
}

float Body::GetLinearDamping() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::BodyLockRead lock(physics->GetSystem().GetBodyLockInterface(), bodyID);
      if (lock.Succeeded()) {
        if (const JPH::MotionProperties* motionProperties = lock.GetBody().GetMotionProperties()) {
          return motionProperties->GetLinearDamping();
        }
      }
    }
  }
  return bodyCreationSettings.mLinearDamping;
}

float Body::GetAngularDamping() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::BodyLockRead lock(physics->GetSystem().GetBodyLockInterface(), bodyID);
      if (lock.Succeeded()) {
        if (const JPH::MotionProperties* motionProperties = lock.GetBody().GetMotionProperties()) {
          return motionProperties->GetAngularDamping();
        }
      }
}
  }
  return bodyCreationSettings.mAngularDamping;
}

JPH::EMotionType Body::GetMotionType() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().GetMotionType(bodyID);
    }
  }
  return bodyCreationSettings.mMotionType;
}

bool Body::IsActive() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().IsActive(bodyID);
    }
  }
  return false;
}

bool Body::IsSensor() const {
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::BodyLockRead lock(physics->GetSystem().GetBodyLockInterface(), bodyID);
      if (lock.Succeeded()) {
        return lock.GetBody().IsSensor();
      }
    }
  }
  return bodyCreationSettings.mIsSensor;
}

void Body::SetShape(JPH::ShapeRefC shape) {
      if (!bodyCreated) {
        spdlog::warn("Tried setting the shape of a body that hasn't been created yet");
        return;
      }
      
      System* physics = GetScene()->GetComponent<System>();
      if (!physics) {
        spdlog::warn("Tried setting the shape of a body without a `PhysicsComponent`");
      }

      physics->GetBodyInterface().SetShape(bodyID, shape, true, JPH::EActivation::Activate);
}

void Body::SetCollisionLayerAndMask(uint32_t layer, uint32_t mask) {
  collisionLayer = layer;
  collisionMask = mask;
  if (bodyCreated) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::CollisionGroup group(physics->GetLayerGroupFilter(), layer, mask);

      if (addedToWorld) physics->GetBodyInterface().RemoveBody(bodyID);

      physics->GetBodyInterface().SetCollisionGroup(bodyID, group);

      if (addedToWorld) physics->GetBodyInterface().AddBody(bodyID, JPH::EActivation::Activate);
    }
  }
}

void Body::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, uint32_t mask) {
  uint32_t combinedLayer = 0;
  for (uint32_t l : layers) combinedLayer |= (1 << l);

  SetCollisionLayerAndMask(combinedLayer, mask);
}

void Body::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, std::initializer_list<uint32_t> collideWithLayers) {
  uint32_t combinedLayer = 0;
  for (uint32_t l : layers) combinedLayer |= (1 << l);

  uint32_t combinedMask = 0;
  for (uint32_t l : collideWithLayers) combinedMask |= (1 << l);

  SetCollisionLayerAndMask(combinedLayer, combinedMask);
}

void Body::SetPosition(const glm::vec3& position) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting position on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetPosition(bodyID, JPH::RVec3(position.x, position.y, position.z), JPH::EActivation::Activate);
  }
}

void Body::SetRotation(const glm::quat& rotation) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting rotation on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetRotation(bodyID, JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::EActivation::Activate);
  }
}

void Body::SetLinearVelocity(const glm::vec3& velocity) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting linear velocity on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetLinearVelocity(bodyID, JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }
}

void Body::SetAngularVelocity(const glm::vec3& velocity) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting angular velocity on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetAngularVelocity(bodyID, JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }
}

void Body::SetFriction(const float friction) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting friction on a body that hasn't been created yet");
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetFriction(bodyID, friction);
  }
}

void Body::SetRestitution(const float restitution) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting restitution on a body that hasn't been created yet");
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetRestitution(bodyID, restitution);
  }
}

void Body::SetGravityFactor(const float factor) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting gravity factor on a body that hasn't been created yet");
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetGravityFactor(bodyID, factor);
  }
}

void Body::SetLinearDamping(float damping) {
  bodyCreationSettings.mAngularDamping = damping;
  if (!bodyCreated) return;
  if (System* physics = GetScene()->GetComponent<System>()) {
    JPH::BodyLockWrite lock(physics->GetSystem().GetBodyLockInterface(), bodyID);

    if (lock.Succeeded()) {
      if (JPH::MotionProperties* motionProperties = lock.GetBody().GetMotionProperties()) {
        motionProperties->SetLinearDamping(damping);
      }
    }
  }
}

void Body::SetAngularDamping(float damping) {
  bodyCreationSettings.mAngularDamping = damping;
  if (!bodyCreated) return;
  if (System* physics = GetScene()->GetComponent<System>()) {
    JPH::BodyLockWrite lock(physics->GetSystem().GetBodyLockInterface(), bodyID);

    if (lock.Succeeded()) {
      if (JPH::MotionProperties* motionProperties = lock.GetBody().GetMotionProperties()) {
        motionProperties->SetAngularDamping(damping);
      }
    }
  }
}

void Body::SetMotionType(JPH::EMotionType motionType) {
  bodyCreationSettings.mMotionType = motionType;
  if (!bodyCreated) return;
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetMotionType(bodyID, motionType, JPH::EActivation::Activate);
  }
}

void Body::SetActivationState(const bool activation) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting the activation state on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    if (activation) {
      physics->GetBodyInterface().ActivateBody(bodyID);
    } else {
      physics->GetBodyInterface().DeactivateBody(bodyID);
    }
  }
}

void Body::SetIsSensor(const bool isSensor) {
  bodyCreationSettings.mIsSensor = isSensor;
  if (!bodyCreated) return;
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetIsSensor(bodyID, isSensor);
  }
}

void Body::ApplyForce(const glm::vec3& force) {
  if (!bodyCreated) {
    spdlog::warn("Tried applying force to a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().AddForce(bodyID, JPH::Vec3(force.x, force.y, force.z));
  }
}

void Body::ApplyImpulse(const glm::vec3& impulse) {
  if (!bodyCreated) {
    spdlog::warn("Tried applying an impulse to a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().AddImpulse(bodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
  }
}

void Body::ApplyTorque(const glm::vec3& torque) {
  if (!bodyCreated) {
    spdlog::warn("Tried applying torque to a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().AddTorque(bodyID, JPH::Vec3(torque.x, torque.y, torque.z));
  }
}

void Body::ApplyAngularImpulse(const glm::vec3& impulse) {
  if (!bodyCreated) {
    spdlog::warn("Tried applying force to a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().AddAngularImpulse(bodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
  }
}

void Body::Awake() {
  System* physics = GetScene()->GetComponent<System>();
  if (!physics) {
      spdlog::warn("Tried waking up a physics object without a PhysicsComponent");
      return;
  }

  JPH::RVec3 position = JPH::RVec3(0.0_r, 0.0_r, 0.0_r);
  JPH::Quat rotation = JPH::Quat::sIdentity();

  SceneNode* node = GetNode();

  SceneTransform::PositionAccess nodePosition = node->GetTransform().GlobalTransform().Position();
  SceneTransform::RotationAccess nodeRotation = node->GetTransform().GlobalTransform().Rotation();
  SceneTransform::ScaleAccess nodeScale = node->GetTransform().GlobalTransform().Scale();

  position = JPH::RVec3(nodePosition.x, nodePosition.y, nodePosition.z);
  rotation = JPH::Quat(nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w);

  const float epsilon = 1.0e-4f;
  bool isScaled = glm::abs(nodeScale.x - 1.0f) > epsilon || 
    glm::abs(nodeScale.y - 1.0f) > epsilon || 
    glm::abs(nodeScale.z - 1.0f) > epsilon;

  if (nodeScale.value != glm::vec3(1.0f)) {
    if (glm::abs(nodeScale.x - nodeScale.y) > epsilon || glm::abs(nodeScale.y - nodeScale.z) > epsilon) {
      spdlog::warn("PhysicsObject: Non-uniform scaling, will fail if applied to a Capsule/Sphere shapes");
    }

    const JPH::ShapeSettings* baseSettings = bodyCreationSettings.GetShapeSettings();

    if (baseSettings != nullptr) {
      // ! Doesn't check whether the scale is valid for the shape !
      JPH::ScaledShapeSettings* scaledSettings = new JPH::ScaledShapeSettings(
        baseSettings,
        JPH::Vec3Arg(nodeScale.x, nodeScale.y, nodeScale.z));
      bodyCreationSettings.SetShapeSettings(scaledSettings);
      };
    }

  bodyCreationSettings.mPosition = position;
  bodyCreationSettings.mRotation = rotation;

  bodyCreationSettings.mUserData = reinterpret_cast<JPH::uint64>(dynamic_cast<GameObject*>(this));

  bodyCreationSettings.mCollisionGroup = JPH::CollisionGroup(physics->GetLayerGroupFilter(), collisionLayer, collisionMask);

  JPH::Body* body = physics->GetBodyInterface().CreateBody(bodyCreationSettings);
  if (!body) {
    spdlog::error("Failed to create a Jolt body");
    bodyCreated = false;
    return;
  }
  bodyID = body->GetID();
  bodyCreated = !bodyID.IsInvalid();
}

void Body::OnEnable() {
  if (!bodyCreated) {
    spdlog::warn("Tried enabling a body that hasn't been created yet");
    return;
  }

  System* physics = GetScene()->GetComponent<System>();
  physics->GetBodyInterface().AddBody(bodyID, JPH::EActivation::Activate);
  addedToWorld = true;
}

void Body::OnDisable() {
  if (!bodyCreated) {
    spdlog::warn("Tried disabling a body that hasn't been created yet");
    return;
  }

  System* physics = GetScene()->GetComponent<System>();
  if (physics) {
    physics->GetBodyInterface().RemoveBody(bodyID);
    addedToWorld = false;
  }
}

void Body::DrawImGui() {
  if (ImGui::TreeNode("Physics Collision")) {
    const float size = ImGui::CalcTextSize("00").x;

    ImGui::Text("Collision Layer");
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 8; x++) {
        if (x > 0) ImGui::SameLine();
        uint32_t bit = y * 8 + x;
        ImGui::PushID(bit + 100);

        bool isSet = (collisionLayer & (1u << bit)) != 0;
        if (ImGui::Selectable(std::to_string(bit).c_str(), isSet, 0, ImVec2(size, size))) {
          SetCollisionLayerAndMask(collisionLayer ^ (1u << bit), collisionMask);
        }
        ImGui::PopID();
      }
    }

    ImGui::Text("Collision Mask");
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 8; x++) {
        if (x > 0) ImGui::SameLine();
        uint32_t bit = y * 8 + x;
        ImGui::PushID(bit + 200);

        bool isSet = (collisionMask & (1 << bit)) != 0;
        if (ImGui::Selectable(std::to_string(bit).c_str(), isSet, 0, ImVec2(size, size))) {
          SetCollisionLayerAndMask(collisionLayer, collisionMask ^ (1 << bit));
        }
        ImGui::PopID();
      }
    }
    ImGui::TreePop();
  }
}
}
