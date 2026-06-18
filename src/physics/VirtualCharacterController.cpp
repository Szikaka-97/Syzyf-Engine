#include "physics/VirtualCharacterController.h"
#include "physics/System.h"
#include <Serialization.h>
#include <Mesh.h>
#include <physics/Helpers.h>

#include "Jolt/Physics/Collision/Shape/Shape.h"
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <imgui.h>

enum class BodyKind {
  Sphere,
  Box,
  Capsule,
  Plane,
  ConvexHullMesh,
  Mesh,
};

namespace Physics {
class VirtualCharacterBodyFilter : public JPH::BodyFilter {
private:
    JPH::BodyInterface& bodyInterface;
    uint32_t charLayer;
    uint32_t charMask;
public:
    VirtualCharacterBodyFilter(JPH::BodyInterface& bi, uint32_t layer, uint32_t mask)
        : bodyInterface(bi), charLayer(layer), charMask(mask) {}

    bool ShouldCollide(const JPH::BodyID& inBodyID) const override {
        JPH::CollisionGroup group = bodyInterface.GetCollisionGroup(inBodyID);
        
        uint32_t bodyLayer = group.GetGroupID();
        uint32_t bodyMask = group.GetSubGroupID();
        
        if ((charMask & bodyLayer) == 0) return false;
        
        if ((bodyMask & charLayer) == 0) return false;

        return true;
    }
    
    bool ShouldCollideLocked(const JPH::Body& inBody) const override {
        JPH::CollisionGroup group = inBody.GetCollisionGroup();
        
        uint32_t bodyLayer = group.GetGroupID();
        uint32_t bodyMask = group.GetSubGroupID();

        if ((charMask & bodyLayer) == 0) return false;
        if ((bodyMask & charLayer) == 0) return false;
        
        return true;
    }
};

VirtualCharacterController::VirtualCharacterController():
characterSettings(new JPH::CharacterVirtualSettings()) { }

VirtualCharacterController::VirtualCharacterController(const JPH::Ref<JPH::CharacterVirtualSettings>& settings) : characterSettings(settings) {}

VirtualCharacterController::~VirtualCharacterController() {}

JPH::Ref<JPH::CharacterVirtual> VirtualCharacterController::GetCharacter() const {
    return this->character;
}

void VirtualCharacterController::Move(const glm::vec3& velocity, float deltaTime) {
  if (!MathHelpers::IsValid(velocity)) {
      spdlog::error("Physics::VirtualCharacterController: Attempted to move with NaN or Inf velocity");
      return;
  }

  System* physics = GetScene()->GetComponent<System>();
  if (!physics || !this->character) {
    spdlog::error("VirtualCharacterController: Move: Tried calling move without a system/on an invalid character");
    return;
  }

  this->character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));

  VirtualCharacterBodyFilter bodyFilter(physics->GetBodyInterface(), this->collisionLayer, this->collisionMask);

  JPH::ObjectLayer joltObjectLayer = Physics::Layers::MOVING;

  this->character->Update(
    deltaTime,
    physics->GetJoltSystem()->GetGravity() * this->gravityFactor,
    physics->GetJoltSystem()->GetDefaultBroadPhaseLayerFilter(joltObjectLayer),
    physics->GetJoltSystem()->GetDefaultLayerFilter(joltObjectLayer),
    bodyFilter,
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

// Returns the character's mass
//  returns zero if the character is a nullptr
float VirtualCharacterController::GetMass() const {
    if (this->character != nullptr) {
        return this->character->GetMass();
    }
    return 0.0f;
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

void VirtualCharacterController::SetCollisionLayerAndMask(uint32_t layer, uint32_t mask) {
    this->collisionLayer = layer;
    this->collisionMask = mask;
}

void VirtualCharacterController::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, uint32_t mask) {
    uint32_t combinedLayer = 0;
    for (uint32_t l : layers) combinedLayer |= (1 << l);
    SetCollisionLayerAndMask(combinedLayer, mask);
}

void VirtualCharacterController::SetCollisionLayerAndMask(std::initializer_list<uint32_t> layers, std::initializer_list<uint32_t> collideWithLayers) {
    uint32_t combinedMask = 0;
    for (uint32_t l : collideWithLayers) combinedMask |= (1 << l);
    SetCollisionLayerAndMask(layers, combinedMask);
}

void VirtualCharacterController::SetPosition(const glm::vec3& position) {
  if (!MathHelpers::IsValid(position)) {
      spdlog::error("Physics::VirtualCharacterController: Attempted to set NaN or Inf position");
      return;
  }
  if (this->character) {
    this->character->SetPosition(JPH::RVec3(position.x, position.y, position.z));
    this->GetTransform().GlobalTransform().Position() = position;
  }
}

void VirtualCharacterController::SetRotation(const glm::quat& rotation) {
  if (!MathHelpers::IsValid(rotation)) {
      spdlog::error("Physics::VirtualCharacterController: Attempted to set NaN or Inf rotation");
      return;
  }
  if (this->character) {
    this->character->SetRotation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w));
    this->GetTransform().GlobalTransform().Rotation() = rotation;
  }
}

void VirtualCharacterController::SetGravityFactor(float factor) {
  this->gravityFactor = factor;
}

void VirtualCharacterController::SetLinearVelocity(const glm::vec3& velocity) {
    if (!MathHelpers::IsValid(velocity)) {
        spdlog::error("Physics::VirtualCharacterController: Attempted to set NaN or Inf linear velocity");
        return;
    }
    if (this->character != nullptr) {
        this->character->SetLinearVelocity(JPH::RVec3(velocity.x, velocity.y, velocity.z));
    } else {
        spdlog::warn("Physics::VirtualCharacterController: Tried setting linear velocity on an invalid character controller");
    }
}

void VirtualCharacterController::SetMass(const float mass) {
    if (this->character != nullptr) {
        this->character->SetMass(mass);
    } else {
        spdlog::warn("Physics::VirtualCharacterController: Tried setting mass on an invalid character controller");
    }
}

void VirtualCharacterController::SyncToNode() {
    if (this->character) {
        glm::vec3 position = this->GetTransform().GlobalTransform().Position().Value();
        glm::quat rotation = this->GetTransform().GlobalTransform().Rotation().Value();
        glm::vec3 scale = this->GetTransform().GlobalTransform().Scale().Value();

        if (!MathHelpers::IsValid(position) || !MathHelpers::IsValid(rotation) || !MathHelpers::IsValid(scale)) {
            spdlog::error("Physics::VirtualCharacterController::SyncToNode: Node has invalid NaN or Inf transform. Skipping sync.");
            return;
        }

        this->SetPosition(position);
        this->SetRotation(rotation);

        if (!glm::all(glm::epsilonEqual(scale, glm::vec3(1.0f), glm::epsilon<float>()))) {
            //commented out because of spam
            // spdlog::warn("Physics::VirtualCharacterController::SyncToNode: Scaling virtual character controllers isn't supported");
        }
    }
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

  this->character = new JPH::CharacterVirtual(this->characterSettings, position, rotation, physics->GetJoltSystem());
}

// // Make consistent with body
void VirtualCharacterController::DrawImGui() {
  // if (ImGui::TreeNode("Virtual Character")) {
  //   int layer = static_cast<int>(this->collisionLayer);
  //   if (ImGui::InputInt("Collision Layer", &layer)) {
  //     this->SetCollisionLayer(static_cast<uint32_t>(layer));
  //   }
  //   ImGui::TreePop();
  // }
}

json VirtualCharacterController::Serialize() const {
  json data;

  data["mMass"] = this->characterSettings->mMass;
  data["mMaxStrength"] = this->characterSettings->mMaxStrength;
  data["mBackFaceMode"] = this->characterSettings->mBackFaceMode;
  data["mPredictiveContactDistance"] = this->characterSettings->mPredictiveContactDistance;
  data["mMaxCollisionIterations"] = this->characterSettings->mMaxCollisionIterations;
  data["mMaxConstraintIterations"] = this->characterSettings->mMaxConstraintIterations;
  data["mMinTimeRemaining"] = this->characterSettings->mMinTimeRemaining;
  data["mCollisionTolerance"] = this->characterSettings->mCollisionTolerance;
  data["mCharacterPadding"] = this->characterSettings->mCharacterPadding;
  data["mMaxNumHits"] = this->characterSettings->mMaxNumHits;
  data["mHitReductionCosMaxAngle"] = this->characterSettings->mHitReductionCosMaxAngle;
  data["mPenetrationRecoverySpeed"] = this->characterSettings->mPenetrationRecoverySpeed;
  data["mShapeOffset"] = Serialization::Serialize(
    glm::vec3(
      this->characterSettings->mShapeOffset.GetX(),
      this->characterSettings->mShapeOffset.GetY(),
      this->characterSettings->mShapeOffset.GetZ()
    )
  );

  json shapeData;

  const JPH::Shape* shape = this->characterSettings->mShape;

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
      shapeData["kind"] = BodyKind::ConvexHullMesh;
      shapeData["mesh"] = ((Mesh*) mesh->GetUserData())->GetPath();

      break;
    }

    spdlog::error("Failed to create shape settings for body on node {}", GetNode()->GetName());
  } while (false); // Because gotos are for losers

  data["shape"] = shapeData;

  return data;
}

void VirtualCharacterController::Deserialize(const json& data) {
  this->characterSettings->mMass = data["mMass"];
  this->characterSettings->mMaxStrength = data["mMaxStrength"];
  this->characterSettings->mBackFaceMode = data["mBackFaceMode"];
  this->characterSettings->mPredictiveContactDistance = data["mPredictiveContactDistance"];
  this->characterSettings->mMaxCollisionIterations = data["mMaxCollisionIterations"];
  this->characterSettings->mMaxConstraintIterations = data["mMaxConstraintIterations"];
  this->characterSettings->mMinTimeRemaining = data["mMinTimeRemaining"];
  this->characterSettings->mCollisionTolerance = data["mCollisionTolerance"];
  this->characterSettings->mCharacterPadding = data["mCharacterPadding"];
  this->characterSettings->mMaxNumHits = data["mMaxNumHits"];
  this->characterSettings->mHitReductionCosMaxAngle = data["mHitReductionCosMaxAngle"];
  this->characterSettings->mPenetrationRecoverySpeed = data["mPenetrationRecoverySpeed"];
  
  glm::vec3 shapeOffset = Serialization::Deserialize<glm::vec3>(data["mShapeOffset"]);
  this->characterSettings->mShapeOffset = JPH::Vec3(shapeOffset.x, shapeOffset.y, shapeOffset.z);

  json shapeData = data["shape"];

  JPH::ShapeRefC characterShape = nullptr;

  switch (shapeData["kind"].get<BodyKind>()) {
  case BodyKind::Sphere: {
    characterShape = Physics::SphereShape(shapeData["radius"]);
    
    break;
  }
  case BodyKind::Box: {
    glm::vec3 halfExtents = Serialization::Deserialize<glm::vec3>(shapeData["halfExtent"]);
    characterShape = Physics::BoxShape(halfExtents);

    break;
  }
  case BodyKind::Capsule: {
    characterShape = Physics::CapsuleShape(
      shapeData["halfHeight"],
      shapeData["radius"]
    );

    break;
  }
  case BodyKind::Plane: {
    glm::vec3 nrm = Serialization::Deserialize<glm::vec3>(shapeData["normal"]);
    characterShape = Physics::PlaneShape(nrm);
    break;
  }
  case BodyKind::ConvexHullMesh: {
    Mesh* hullMesh = ResourceDatabase::Global->Get<Mesh>(shapeData["mesh"]);

    characterShape = Physics::ConvexHullMeshShape(hullMesh);

    break;
  }
  case BodyKind::Mesh: {
    Mesh* bodyMesh = ResourceDatabase::Global->Get<Mesh>(shapeData["mesh"]);

    characterShape = Physics::MeshShape(bodyMesh);

    break;
  }
  }

  if (characterShape) {
    this->characterSettings->mShape = characterShape;
  }
}

}