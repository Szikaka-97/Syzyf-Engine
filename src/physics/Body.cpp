#include "physics/Body.h"
#include "physics/System.h"

#include "GameObject.h"
#include <Jolt/Jolt.h>
#include "Jolt/Math/Real.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <Serialization.h>
#include <Mesh.h>
#include <physics/Helpers.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

namespace Physics {
using namespace JPH::literals;

enum class BodyKind {
  Sphere,
  Box,
  Capsule,
  Plane,
  ConvexHullMesh,
  Mesh,
};

Body::Body(const JPH::BodyCreationSettings& settings): bodyCreationSettings(settings) {}

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
      JPH::BodyLockRead lock(physics->GetJoltSystem()->GetBodyLockInterface(), bodyID);
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
      JPH::BodyLockRead lock(physics->GetJoltSystem()->GetBodyLockInterface(), bodyID);
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
      JPH::BodyLockRead lock(physics->GetJoltSystem()->GetBodyLockInterface(), bodyID);
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

  if (!MathHelpers::IsValid(position)) {
      spdlog::error("Physics::Body: Attemped to set NaN or Inf position");
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

  if (!MathHelpers::IsValid(rotation)) {
      spdlog::error("Physics::Body: Attemped to set NaN or Inf rotation");
      return;
  }

  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetRotation(bodyID, JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w).Normalized(), JPH::EActivation::Activate);
  }
}

void Body::SetLinearVelocity(const glm::vec3& velocity) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting linear velocity on a body that hasn't been created yet");
    return;
  }

  if (!MathHelpers::IsValid(velocity)) {
      spdlog::error("Physics::Body: Attemped to set NaN or Inf velocity");
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

  if (!MathHelpers::IsValid(velocity)) {
      spdlog::error("Physics::Body: Attemped to set NaN or Inf angular velocity");
      return;
  }

  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetAngularVelocity(bodyID, JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }
}

void Body::SetFriction(const float friction) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting friction on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetFriction(bodyID, friction);
  }
}

void Body::SetRestitution(const float restitution) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting restitution on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetRestitution(bodyID, restitution);
  }
}

void Body::SetGravityFactor(const float factor) {
  if (!bodyCreated) {
    spdlog::warn("Tried setting gravity factor on a body that hasn't been created yet");
    return;
  }
  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().SetGravityFactor(bodyID, factor);
  }
}

void Body::SetLinearDamping(float damping) {
  bodyCreationSettings.mLinearDamping = damping;
  if (!bodyCreated) return;
  if (System* physics = GetScene()->GetComponent<System>()) {
    JPH::BodyLockWrite lock(physics->GetJoltSystem()->GetBodyLockInterface(), bodyID);

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
    JPH::BodyLockWrite lock(physics->GetJoltSystem()->GetBodyLockInterface(), bodyID);

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

  if (!MathHelpers::IsValid(force)) {
      spdlog::error("Physics::Body: Attemped to set NaN or Inf position");
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

  if (!MathHelpers::IsValid(impulse)) {
      spdlog::error("Physics::Body: Attemped to apply NaN or Inf impulse");
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

  if (!MathHelpers::IsValid(torque)) {
      spdlog::error("Physics::Body: Attemped to apply NaN or Inf torque");
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

  if (!MathHelpers::IsValid(impulse)) {
      spdlog::error("Physics::Body: Attemped to apply NaN or Inf angular impulse");
      return;
  }

  if (System* physics = GetScene()->GetComponent<System>()) {
    physics->GetBodyInterface().AddAngularImpulse(bodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
  }
}

// Syncs the node when moving it in the editor
void Body::SyncToNode() {
    if (!bodyCreated) {
        // spdlog::warn("Tried syncing a body that hasn't been created yet");
        return;
    }
    
    glm::vec3 position = GetNode()->GlobalTransform().Position().Value();
    glm::quat rotation = GetNode()->GlobalTransform().Rotation().Value();
    glm::vec3 scale = GetNode()->GlobalTransform().Scale().Value();

    if (!MathHelpers::IsValid(position) || !MathHelpers::IsValid(rotation) || !MathHelpers::IsValid(scale)) {
        spdlog::error("Physics::Body::SyncToNode: Node has invalid NaN or Inf transform. Skipping sync.");
        return;
    }

    this->SetPosition(position);
    this->SetRotation(rotation);

    if (scale != this->lastScale && this->originalShape != nullptr) {
        JPH::ShapeRefC newShape;

        if (glm::abs(scale.x) < 0.001f) scale.x = scale.x < 0.0f ? -0.001f : 0.001f;
        if (glm::abs(scale.y) < 0.001f) scale.y = scale.y < 0.0f ? -0.001f : 0.001f;
        if (glm::abs(scale.z) < 0.001f) scale.z = scale.z < 0.0f ? -0.001f : 0.001f;
            
        if (scale == glm::vec3(1.0f)) {
            newShape = this->originalShape;
        } else {
            JPH::EShapeSubType subType = this->originalShape->GetSubType();

            if (subType == JPH::EShapeSubType::Sphere || subType == JPH::EShapeSubType::Capsule) {
                float maxScale = std::max({glm::abs(scale.x), glm::abs(scale.y), glm::abs(scale.z)});

                if (glm::abs(scale.x - scale.y) > glm::epsilon<float>() || glm::abs(scale.y - scale.z) > glm::epsilon<float>()) {
                    spdlog::warn("Physics::Body::SyncToNode: Forced uniform scale for Sphere/Capsule");  
                }

                newShape = new JPH::ScaledShape(
                    this->originalShape,
                    JPH::Vec3(maxScale, maxScale, maxScale)
                );
            } else {
                newShape = new JPH::ScaledShape(
                    this->originalShape,
                    JPH::Vec3(scale.x, scale.y, scale.z)
                );
            }
        }

        if (Physics::System* system = this->GetScene()->GetComponent<Physics::System>()) {
            system->GetBodyInterface().SetShape(
                GetBodyID(), 
                newShape, 
                false,
                JPH::EActivation::DontActivate
            );
            this->lastScale = scale;
        }
    }
    // This activates the body after it's been moved
    //  not sure if having this happen while editing won't cause issues
    // the same is true for character controllers
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

  glm::vec3 nodePosition = node->GetTransform().GlobalTransform().Position();
  glm::quat nodeRotation = node->GetTransform().GlobalTransform().Rotation();
  glm::vec3 nodeScale = node->GetTransform().GlobalTransform().Scale();

  position = JPH::RVec3(nodePosition.x, nodePosition.y, nodePosition.z);
  rotation = JPH::Quat(nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w).Normalized();

if (bodyCreationSettings.GetShapeSettings() != nullptr) {
      JPH::Shape::ShapeResult result = bodyCreationSettings.GetShapeSettings()->Create();
      if (result.IsValid()) {
          this->originalShape = result.Get();
      }
  } else if (bodyCreationSettings.GetShape() != nullptr) {
      this->originalShape = bodyCreationSettings.GetShape();
  }

    if (glm::abs(nodeScale.x) < 0.001f) nodeScale.x = nodeScale.x < 0.0f ? -0.001f : 0.001f;
    if (glm::abs(nodeScale.y) < 0.001f) nodeScale.y = nodeScale.y < 0.0f ? -0.001f : 0.001f;
    if (glm::abs(nodeScale.z) < 0.001f) nodeScale.z = nodeScale.z < 0.0f ? -0.001f : 0.001f;

  JPH::ShapeRefC activeShape = this->originalShape;
  if (nodeScale != glm::vec3(1.0f)) {
      JPH::EShapeSubType subType = this->originalShape->GetSubType();

      if (subType == JPH::EShapeSubType::Sphere || subType == JPH::EShapeSubType::Capsule) {
          float maxScale = std::max({glm::abs(nodeScale.x), glm::abs(nodeScale.y), glm::abs(nodeScale.z)});
          activeShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(maxScale, maxScale, maxScale));

          if (glm::abs(nodeScale.x - nodeScale.y) > glm::epsilon<float>() || glm::abs(nodeScale.y - nodeScale.z) > glm::epsilon<float>()) {
              spdlog::warn("PhysicsObject::Awake: Forced uniform scaling for Sphere/Capsule shape");
          }
      } else {
          activeShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(nodeScale.x, nodeScale.y, nodeScale.z));
      }
  }

  bodyCreationSettings.SetShape(activeShape);
  this->lastScale = nodeScale;

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
  if (!bodyCreated || addedToWorld) {
    spdlog::warn("Tried enabling a body that hasn't been created yet, or one which has already been added");
    return;
  }

  System* physics = GetScene()->GetComponent<System>();
  physics->GetBodyInterface().AddBody(bodyID, JPH::EActivation::Activate);
  addedToWorld = true;
}

void Body::OnDisable() {
  if (!bodyCreated || !addedToWorld) {
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

json Body::Serialize() const {
  json data;

  data["collisionLayer"] = this->collisionLayer;
  data["collisionMask"] = this->collisionMask;
  data["mObjectLayer"] = this->bodyCreationSettings.mObjectLayer;
  data["mMotionType"] = this->bodyCreationSettings.mMotionType;
  data["mAllowedDOFs"] = this->bodyCreationSettings.mAllowedDOFs;
  data["mAllowDynamicOrKinematic"] = this->bodyCreationSettings.mAllowDynamicOrKinematic;
  data["mIsSensor"] = this->bodyCreationSettings.mIsSensor;
  data["mCollideKinematicVsNonDynamic"] = this->bodyCreationSettings.mCollideKinematicVsNonDynamic;
  data["mUseManifoldReduction"] = this->bodyCreationSettings.mUseManifoldReduction;
  data["mApplyGyroscopicForce"] = this->bodyCreationSettings.mApplyGyroscopicForce;
  data["mMotionQuality"] = this->bodyCreationSettings.mMotionQuality;
  data["mEnhancedInternalEdgeRemoval"] = this->bodyCreationSettings.mEnhancedInternalEdgeRemoval;
  data["mAllowSleeping"] = this->bodyCreationSettings.mAllowSleeping;
  data["mFriction"] = this->bodyCreationSettings.mFriction;
  data["mRestitution"] = this->bodyCreationSettings.mRestitution;
  data["mLinearDamping"] = this->bodyCreationSettings.mLinearDamping;
  data["mAngularDamping"] = this->bodyCreationSettings.mAngularDamping;
  data["mMaxLinearVelocity"] = this->bodyCreationSettings.mMaxLinearVelocity;
  data["mMaxAngularVelocity"] = this->bodyCreationSettings.mMaxAngularVelocity;
  data["mGravityFactor"] = this->bodyCreationSettings.mGravityFactor;

  data["mOverrideMassProperties"] = this->bodyCreationSettings.mOverrideMassProperties;
  data["mInertiaMultiplier"] = this->bodyCreationSettings.mInertiaMultiplier;
  data["mMass"] = this->bodyCreationSettings.mOverrideMassProperties != JPH::EOverrideMassProperties::CalculateMassAndInertia ? this->bodyCreationSettings.mMassPropertiesOverride.mMass : 0;

  json shapeData;

  const JPH::Shape* shape = this->bodyCreationSettings.GetShape();

  if (dynamic_cast<const JPH::DecoratedShape *>(shape)) {
    shape = dynamic_cast<const JPH::DecoratedShape *>(shape)->GetInnerShape();
  }

  do {
    const JPH::SphereShape* sphere = dynamic_cast<const JPH::SphereShape*>(shape);

    if (sphere) {
      shapeData["kind"] = BodyKind::Sphere;
      shapeData["radius"] = sphere->GetRadius();

      break;
    }

    const JPH::BoxShape* box = dynamic_cast<const JPH::BoxShape*>(shape);

    if (box) {
      shapeData["kind"] = BodyKind::Box;
      JPH::Vec3 ext = box->GetHalfExtent();
      shapeData["halfExtent"] = Serialization::Serialize(glm::vec3(ext.GetX(), ext.GetY(), ext.GetZ()));

      break;
    }

    const JPH::CapsuleShape* capsule = dynamic_cast<const JPH::CapsuleShape*>(shape);

    if (capsule) {
      shapeData["kind"] = BodyKind::Capsule;
      shapeData["radius"] = capsule->GetRadius();
      shapeData["halfHeight"] = capsule->GetHalfHeightOfCylinder();

      break;
    }

    const JPH::PlaneShape* plane = dynamic_cast<const JPH::PlaneShape*>(shape);

    if (plane) {
      shapeData["kind"] = BodyKind::Plane;
      JPH::Vec3 nrm = plane->GetSurfaceNormal(JPH::SubShapeID(), JPH::Vec3::sZero());
      shapeData["normal"] = Serialization::Serialize(glm::vec3(nrm.GetX(), nrm.GetY(), nrm.GetZ()));

      break;
    }

    const JPH::ConvexHullShape* hull = dynamic_cast<const JPH::ConvexHullShape*>(shape);

    if (hull) {
      shapeData["kind"] = BodyKind::ConvexHullMesh;
      shapeData["mesh"] = ((Mesh*) hull->GetUserData())->GetPath();

      break;
    }

    const JPH::MeshShape* mesh = dynamic_cast<const JPH::MeshShape*>(shape);

    if (mesh) {
      shapeData["kind"] = BodyKind::Mesh;
      shapeData["mesh"] = ((Mesh*) mesh->GetUserData())->GetPath();

      break;
    }

    spdlog::error("Failed to create shape settings for body on node {}", GetNode()->GetName());
  } while (false); // Because gotos are for losers

  data["shape"] = shapeData;

  return data;
}

void Body::Deserialize(const json& data) {
  this->collisionLayer = data["collisionLayer"];
  this->collisionMask = data["collisionMask"];
  this->bodyCreationSettings.mObjectLayer = data["mObjectLayer"];
  this->bodyCreationSettings.mMotionType = data["mMotionType"];
  this->bodyCreationSettings.mAllowedDOFs = data["mAllowedDOFs"];
  this->bodyCreationSettings.mAllowDynamicOrKinematic = data["mAllowDynamicOrKinematic"];
  this->bodyCreationSettings.mIsSensor = data["mIsSensor"];
  this->bodyCreationSettings.mCollideKinematicVsNonDynamic = data["mCollideKinematicVsNonDynamic"];
  this->bodyCreationSettings.mUseManifoldReduction = data["mUseManifoldReduction"];
  this->bodyCreationSettings.mApplyGyroscopicForce = data["mApplyGyroscopicForce"];
  this->bodyCreationSettings.mMotionQuality = data["mMotionQuality"];
  this->bodyCreationSettings.mEnhancedInternalEdgeRemoval = data["mEnhancedInternalEdgeRemoval"];
  this->bodyCreationSettings.mAllowSleeping = data["mAllowSleeping"];
  this->bodyCreationSettings.mFriction = data["mFriction"];
  this->bodyCreationSettings.mRestitution = data["mRestitution"];
  this->bodyCreationSettings.mLinearDamping = data["mLinearDamping"];
  this->bodyCreationSettings.mAngularDamping = data["mAngularDamping"];
  this->bodyCreationSettings.mMaxLinearVelocity = data["mMaxLinearVelocity"];
  this->bodyCreationSettings.mMaxAngularVelocity = data["mMaxAngularVelocity"];
  this->bodyCreationSettings.mGravityFactor = data["mGravityFactor"];

  this->bodyCreationSettings.mOverrideMassProperties = data["mOverrideMassProperties"];
  this->bodyCreationSettings.mInertiaMultiplier = data["mInertiaMultiplier"];

  if (this->bodyCreationSettings.mOverrideMassProperties != JPH::EOverrideMassProperties::CalculateMassAndInertia) {
    this->bodyCreationSettings.mMassPropertiesOverride.mMass = data["mMass"];
  }

  json shapeData = data["shape"];

  switch (shapeData["kind"].get<BodyKind>()) {
  case BodyKind::Sphere: {
    this->bodyCreationSettings.SetShape(Physics::SphereShape(shapeData["radius"]));
    
    break;
  }
  case BodyKind::Box: {
    glm::vec3 halfExtents = Serialization::Deserialize<glm::vec3>(shapeData["halfExtent"]);
    this->bodyCreationSettings.SetShape(Physics::BoxShape(halfExtents));

    break;
  }
  case BodyKind::Capsule: {
    this->bodyCreationSettings.SetShape(Physics::CapsuleShape(
      shapeData["halfHeight"],
      shapeData["radius"]
    ));

    break;
  }
  case BodyKind::Plane: {
    glm::vec3 nrm = Serialization::Deserialize<glm::vec3>(shapeData["normal"]);
    this->bodyCreationSettings.SetShape(Physics::PlaneShape(nrm));
    break;
  }
  case BodyKind::ConvexHullMesh: {
    Mesh* hullMesh = ResourceDatabase::Global->Get<Mesh>(shapeData["mesh"]);

    this->bodyCreationSettings.SetShape(Physics::ConvexHullMeshShape(hullMesh));

    break;
  }
  case BodyKind::Mesh: {
    Mesh* bodyMesh = ResourceDatabase::Global->Get<Mesh>(shapeData["mesh"]);

    this->bodyCreationSettings.SetShape(Physics::MeshShape(bodyMesh));

    break;
  }
  }

}
}

