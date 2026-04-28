#include "physics/CharacterController.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "Jolt/Physics/EActivation.h"
#include "physics/System.h"
#include <spdlog/spdlog.h>
#include <imgui.h>

namespace Physics {
CharacterController::CharacterController(const JPH::Ref<JPH::CharacterSettings>& settings): characterSettings(settings) {
  spdlog::info("PhysicsCharacter: Added a character controller");
}

CharacterController::~CharacterController() {
  delete this->character;
}

JPH::BodyID CharacterController::GetBodyID() const {
  if (this->character) {
    return this->character->GetBodyID();
  }
  return JPH::BodyID();
}

uint32_t CharacterController::GetCollisionLayer() const {
  return this->collisionLayer;
}

uint32_t CharacterController::GetCollisionMask() const {
  return this->collisionMask;
}

JPH::Character* CharacterController::GetCharacter() const {
  return this->character;
}

glm::vec3 CharacterController::GetLinearVelocity() const {
  if (this->character) {
    JPH::Vec3 velocity = this->character->GetLinearVelocity();
    return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
  }
  return glm::vec3(0.0f);
}

glm::vec3 CharacterController::GetPosition() const {
  if (this->character) {
    JPH::RVec3 position = this->character->GetPosition();
    return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
  }
  return glm::vec3(0.0f);
}

glm::quat CharacterController::GetRotation() const {
  if (this->character) {
    JPH::Quat rotation = this->character->GetRotation();
    return glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
  }
  return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

float CharacterController::GetGravityFactor() const {
  if (this->character) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      return physics->GetBodyInterface().GetGravityFactor(this->character->GetBodyID());
    }
  }
  return 1.0f;
}

bool CharacterController::IsSupported() const {
  if (this->character) {
    return this->character->IsSupported();
  }
  return false;
}

JPH::BodyID CharacterController::GetGroundBodyID() const {
  if (this->character) {
    return this->character->GetBodyID();
  }
  return JPH::BodyID();
}

SceneNode* CharacterController::GetGroundObject() const {
  if (this->character) {
    uint64_t userData = this->character->GetGroundUserData();
    GameObject* object = reinterpret_cast<GameObject*>(userData);
    if (object) {
      return object->GetNode();
    }
  }
  return nullptr;
}

glm::vec3 CharacterController::GetGroundPosition() const {
  if (this->character) {
    JPH::Vec3 position = this->character->GetGroundPosition();
    return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
  }
  return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 CharacterController::GetGroundNormal() const {
  if (this->character) {
    JPH::Vec3 normal = this->character->GetGroundNormal();
    return glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
  }
  return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 CharacterController::GetGroundVelocity() const {
  if (this->character) {
    JPH::Vec3 velocity = this->character->GetGroundVelocity();
    return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
  }
  return glm::vec3(0.0f);
}

JPH::CharacterBase::EGroundState CharacterController::GetGroundState() const {
  if (this->character) {
    return this->character->GetGroundState();
  }
  return JPH::CharacterBase::EGroundState::InAir;
}

void CharacterController::SetCollisionLayerAndMask(uint32_t layer, uint32_t mask) {
  collisionLayer = layer;
  collisionMask = mask;
  
  if (this->character) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      JPH::CollisionGroup group(physics->GetLayerGroupFilter(), layer, mask);

      if (this->IsEnabled()) this->character->RemoveFromPhysicsSystem();

      physics->GetBodyInterface().SetCollisionGroup(this->character->GetBodyID(), group);

      if (this->IsEnabled()) this->character->AddToPhysicsSystem(JPH::EActivation::Activate);
    }
  }
}

void CharacterController::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, uint32_t mask) {
  uint32_t combinedLayer = 0;
  for (uint32_t l : layers) combinedLayer |= (1 << l);
  SetCollisionLayerAndMask(combinedLayer, mask);
}

void CharacterController::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, std::initializer_list<uint32_t> collideWithLayers) {
  uint32_t combinedLayer = 0;
  for (uint32_t l : layers) combinedLayer |= (1 << l);
  
  uint32_t combinedMask = 0;
  for (uint32_t l : collideWithLayers) combinedMask |= (1 << l);
  
  SetCollisionLayerAndMask(combinedLayer, combinedMask);
}

void CharacterController::AddImpulse(const glm::vec3& impulse) {
  if (!MathHelpers::IsValid(impulse)) {
      spdlog::error("Physics::CharacterController: Attempted to add NaN or Inf impulse");
      return;
  }
  if (this->character) {
    this->character->AddImpulse(JPH::Vec3(impulse.x, impulse.y, impulse.z));
  }
}

void CharacterController::SetLinearVelocity(const glm::vec3& velocity) {
  if (!MathHelpers::IsValid(velocity)) {
      spdlog::error("Physics::CharacterController: Attempted to set NaN or Inf velocity");
      return;
  }
  if (this->character) {
    this->character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }
}

void CharacterController::AddLinearVelocity(const glm::vec3& velocity) {
  if (!MathHelpers::IsValid(velocity)) {
      spdlog::error("Physics::CharacterController: Attempted to add NaN or Inf velocity");
      return;
  }
  if (this->character) {
    this->character->AddLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }
}

void CharacterController::SetPosition(const glm::vec3& position) {
  if (!MathHelpers::IsValid(position)) {
      spdlog::error("Physics::CharacterController: Attempted to set NaN or Inf position");
      return;
  }
  if (this->character) {
    this->character->SetPosition(JPH::RVec3(position.x, position.y, position.z));
  }
}

void CharacterController::SetRotation(const glm::quat& rotation) {
  if (!MathHelpers::IsValid(rotation)) {
      spdlog::error("Physics::CharacterController: Attempted to set NaN or Inf rotation");
      return;
  }
  if (this->character) {
    this->character->SetRotation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w));
  }
}

void CharacterController::SetGravityFactor(float factor) {
  if (this->character) {
    if (System* physics = GetScene()->GetComponent<System>()) {
      physics->GetBodyInterface().SetGravityFactor(this->character->GetBodyID(), factor);
    }
  }
}

void CharacterController::SetUp(const glm::vec3& up) {
  if (!MathHelpers::IsValid(up)) {
      spdlog::error("Physics::CharacterController: Attempted to set NaN or Inf up vector");
      return;
  }
  if (this->character) {
    this->character->SetUp(JPH::Vec3(up.x, up.y, up.z));
  }
}

bool CharacterController::SetShape(const JPH::RefConst<JPH::Shape>& shape, float maxPenetrationDepth) {
  if (this->character) {
    System* physics = GetScene()->GetComponent<System>();
    if (physics != nullptr) {
      return this->character->SetShape(shape, maxPenetrationDepth);
    }
  }
  return false;
}

// Syncs the node when moving it in the editor
void CharacterController::SyncToNode() {
    glm::vec3 position = this->GetTransform().GlobalTransform().Position().Value();
    glm::quat rotation = this->GetTransform().GlobalTransform().Rotation().Value();
    glm::vec3 scale = this->GetTransform().GlobalTransform().Scale().Value();

    if (!MathHelpers::IsValid(position) || !MathHelpers::IsValid(rotation) || !MathHelpers::IsValid(scale)) {
        spdlog::error("Physics::CharacterController::SyncToNode: Node has invalid NaN or Inf transform. Skipping sync.");
        return;
    }

    this->SetPosition(position);
    this->SetRotation(rotation);

    if (scale != this->lastScale && this->originalShape != nullptr) {
        JPH::ShapeRefC newShape;

        if (scale == glm::vec3(1.0f)) {
            newShape = this->originalShape;
        } else {
            JPH::EShapeSubType subType = this->originalShape->GetSubType();

            if (subType == JPH::EShapeSubType::Sphere || subType == JPH::EShapeSubType::Capsule) {
                float maxScale = std::max({glm::abs(scale.x), glm::abs(scale.y), glm::abs(scale.z)});
                newShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(maxScale, maxScale, maxScale));

                if (glm::abs(scale.x - scale.y) > glm::epsilon<float>() || glm::abs(scale.y - scale.z) > glm::epsilon<float>()) {
                    spdlog::warn("Physics::CharacterController::SyncToNode: Forced uniform scaling for Sphere/Capsule shape.");                 
                }
            } else {
                newShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(scale.x, scale.y, scale.z));
            }
        }

        if (this->character) {
            this->character->SetShape(newShape, 1.0e-4f);
        }
        this->lastScale = scale;
    }
}
void CharacterController::Awake() {
  System* physics = this->GetScene()->GetComponent<System>();
  if (physics == nullptr) {
    spdlog::warn("Tried waking up a physics character without a PhysicsComponent");
    return;
  }

  glm::vec3 nodePosition = this->GetTransform().GlobalTransform().Position().Value();
  glm::quat nodeRotation = this->GetTransform().GlobalTransform().Rotation().Value();
  glm::vec3 nodeScale = this->GetTransform().GlobalTransform().Scale().Value();

  JPH::RVec3 position = JPH::RVec3(nodePosition.x, nodePosition.y, nodePosition.z);
  JPH::Quat rotation = JPH::Quat(nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w);

  if (this->originalShape == nullptr) {
      this->originalShape = this->characterSettings->mShape;
  }

  JPH::ShapeRefC activeShape = this->originalShape;
  if (nodeScale != glm::vec3(1.0f)) {
      JPH::EShapeSubType subType = this->originalShape->GetSubType();

      if (subType == JPH::EShapeSubType::Sphere || subType == JPH::EShapeSubType::Capsule) {
          float maxScale = std::max({glm::abs(nodeScale.x), glm::abs(nodeScale.y), glm::abs(nodeScale.z)});
          activeShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(maxScale, maxScale, maxScale));

          if (glm::abs(nodeScale.x - nodeScale.y) > glm::epsilon<float>() || glm::abs(nodeScale.y - nodeScale.z) > glm::epsilon<float>()) {
              spdlog::warn("Physics::CharacterController::Awake: Forced uniform scaling for Sphere/Capsule shape.");
          }
      } else {
          activeShape = new JPH::ScaledShape(this->originalShape, JPH::Vec3(nodeScale.x, nodeScale.y, nodeScale.z));
      }
  }

  this->characterSettings->mShape = activeShape;
  this->lastScale = nodeScale;

  this->character = new JPH::Character(this->characterSettings, position, rotation, 0, physics->GetJoltSystem());

  JPH::CollisionGroup group(physics->GetLayerGroupFilter(), collisionLayer, collisionMask);
  physics->GetBodyInterface().SetCollisionGroup(this->character->GetBodyID(), group);

  physics->GetBodyInterface().SetUserData(
    this->character->GetBodyID(),
    reinterpret_cast<JPH::uint64>(dynamic_cast<GameObject*>(this))
  );

  spdlog::info("PhysicsCharacter: A character controller called Awake()");
}

void CharacterController::OnEnable() {
  System* physics = this->GetScene()->GetComponent<System>();
  if (physics == nullptr) {
    spdlog::warn("Tried waking up a physics character without a PhysicsComponent");
    return;
  }

  if (this->character != nullptr) {
    this->character->AddToPhysicsSystem(JPH::EActivation::Activate);
    spdlog::info("PhysicsCharacter: Character controller activated");
  }
}

void CharacterController::OnDisable() {
  if (this->character != nullptr) {
    this->character->RemoveFromPhysicsSystem();
  }
}

void CharacterController::DrawImGui() {
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
          SetCollisionLayerAndMask(collisionLayer ^ (1 << bit), collisionMask);
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

        bool isSet = (collisionMask & (1u << bit)) != 0;
        if (ImGui::Selectable(std::to_string(bit).c_str(), isSet, 0, ImVec2(size, size))) {
          SetCollisionLayerAndMask(collisionLayer, collisionMask ^ (1u << bit));
        }
        ImGui::PopID();
      }
    }
    ImGui::TreePop();
  }
}
}
