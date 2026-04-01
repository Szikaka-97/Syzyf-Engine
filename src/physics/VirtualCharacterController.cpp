#include "physics/VirtualCharacterController.h"
#include "physics/System.h"

#include <imgui.h>

namespace Physics {
VirtualCharacterController::VirtualCharacterController(const JPH::Ref<JPH::CharacterVirtualSettings>& settings) : characterSettings(settings) {}

VirtualCharacterController::~VirtualCharacterController() {}

void VirtualCharacterController::Move(const glm::vec3& velocity, float deltaTime) {
  System* physics = GetScene()->GetComponent<System>();
  if (!physics || !this->character) {
    spdlog::error("VirtualCharacterController: Move: Tried calling move without a system/on an invalid character");
    return;
  }

  this->character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));

  this->character->Update(
    deltaTime,
    physics->GetSystem().GetGravity() * this->gravityFactor,
    physics->GetSystem().GetDefaultBroadPhaseLayerFilter(collisionLayer),
    physics->GetSystem().GetDefaultLayerFilter(collisionLayer),
    { },
    { },
    physics->GetTempAllocator()
  );

  JPH::RVec3 position = this->character->GetPosition();
  this->GetTransform().GlobalTransform().Position() = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
}

glm::vec3 VirtualCharacterController::GetPosition() const {
  if (this->character) {
    JPH::RVec3 position = this->character->GetPosition();
    return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
  }
  return glm::vec3(0.0f);
}

glm::quat VirtualCharacterController::GetRotation() const {
  if (this->character) {
    JPH::Quat rotation = this->character->GetRotation();
    return glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
  }
  return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

float VirtualCharacterController::GetGravityFactor() const {
  return this->gravityFactor;
}

glm::vec3 VirtualCharacterController::GetLinearVelocity() const {
  if (this->character) {
    JPH::Vec3 velocity = this->character->GetLinearVelocity();
    return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
  }
  return glm::vec3(0.0f);
}

JPH::BodyID VirtualCharacterController::GetGroundBodyID() const {
  if (this->character) {
    return this->character->GetGroundBodyID();
  }
  return JPH::BodyID();
}

SceneNode* VirtualCharacterController::GetGroundObject() const {
  if (this->character) {
    uint64_t userData = this->character->GetGroundUserData();
    GameObject* object = reinterpret_cast<GameObject*>(userData);
    if (object) {
      return object->GetNode();
    }
  }
  return nullptr;
}

glm::vec3 VirtualCharacterController::GetGroundNormal() const {
  if (this->character) {
    JPH::Vec3 normal = this->character->GetGroundNormal();
    return glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
  }
  return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 VirtualCharacterController::GetGroundVelocity() const {
  if (this->character) {
    JPH::Vec3 velocity = this->character->GetGroundVelocity();
    return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
  }
  return glm::vec3(0.0f);
}

JPH::CharacterBase::EGroundState VirtualCharacterController::GetGroundState() const {
  if (this->character) {
    return this->character->GetGroundState();
  }
  return JPH::CharacterBase::EGroundState::NotSupported;
}

bool VirtualCharacterController::IsSupported() const {
  if (this->character) {
    return this->character->IsSupported();
  }
  return false;
}

void VirtualCharacterController::SetCollisionLayer(uint32_t layer) {
  this->collisionLayer = layer;
}

void VirtualCharacterController::SetCollisionLayer(std::initializer_list<uint32_t> layers) {
  uint32_t combinedLayer = 0;
  for (uint32_t l : layers) combinedLayer |= (1 << l);
  SetCollisionLayer(combinedLayer);
}

void VirtualCharacterController::SetPosition(const glm::vec3& position) {
  if (this->character) {
    this->character->SetPosition(JPH::RVec3(position.x, position.y, position.z));
    this->GetTransform().GlobalTransform().Position() = position;
  }
}

void VirtualCharacterController::SetRotation(const glm::quat& rotation) {
  if (this->character) {
    this->character->SetRotation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w));
    this->GetTransform().GlobalTransform().Rotation() = rotation;
  }
}

void VirtualCharacterController::SetGravityFactor(float factor) {
  this->gravityFactor = factor;
}

void VirtualCharacterController::Awake() {
  System* physics = this->GetScene()->GetComponent<System>();
  if (physics == nullptr) {
    spdlog::error("VirtualCharacterController: Awake: Tried waking up a virtual character controller without a physics system");
    return;
  }

  glm::vec3 nodePosition = this->GetTransform().GlobalTransform().Position();
  glm::quat nodeRotation = this->GetTransform().GlobalTransform().Rotation();

  JPH::RVec3 position(nodePosition.x, nodePosition.y, nodePosition.z);
  JPH::Quat rotation(nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeRotation.w);

  this->character = new JPH::CharacterVirtual(this->characterSettings, position, rotation, &physics->GetSystem());
}

// Make consistent with body
void VirtualCharacterController::DrawImGui() {
  if (ImGui::TreeNode("Virtual Character")) {
    int layer = static_cast<int>(this->collisionLayer);
    if (ImGui::InputInt("Collision Layer", &layer)) {
      this->SetCollisionLayer(static_cast<uint32_t>(layer));
    }
    ImGui::TreePop();
  }
}
}
