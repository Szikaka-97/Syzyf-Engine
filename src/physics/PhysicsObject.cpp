#include "physics/PhysicsObject.h"

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
#include "physics/PhysicsComponent.h"
#include <spdlog/spdlog.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

PhysicsObject::PhysicsObject() {};

PhysicsObject::PhysicsObject(const JPH::BodyCreationSettings& settings): bodyCreationSettings(settings) {}

JPH::BodyCreationSettings PhysicsObject::Sphere(float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
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

JPH::BodyCreationSettings PhysicsObject::Box(glm::vec3 halfExtent, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  // Should perhaps check here for if the extents are too small
  return JPH::BodyCreationSettings(
      new JPH::BoxShape(JPH::Vec3Arg(halfExtent.x, halfExtent.y, halfExtent.z)),
      JPH::RVec3Arg::sZero(),
      JPH::QuatArg::sIdentity(),
      type,
      layer
  );
}

JPH::BodyCreationSettings PhysicsObject::Capsule(float halfHeight, float radius, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  // !! check whether small size makes this crash here too!
  return JPH::BodyCreationSettings(
      new JPH::CapsuleShape(halfHeight, radius),
      JPH::RVec3Arg::sZero(),
      JPH::QuatArg::sIdentity(),
      type,
      layer
  );    
}

JPH::BodyCreationSettings PhysicsObject::Plane(glm::vec3 normal, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
  return JPH::BodyCreationSettings (
    new JPH::PlaneShape,
    JPH::RVec3Arg::sZero(),
    JPH::QuatArg::sIdentity(),
    type,
    layer
  );
}

JPH::BodyCreationSettings PhysicsObject::ConvexHullMesh(const class Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
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

JPH::BodyCreationSettings PhysicsObject::Mesh(const class Mesh* mesh, const JPH::EMotionType type, const JPH::ObjectLayer layer) {
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

PhysicsObject::~PhysicsObject() {
  if (bodyCreated) {
    PhysicsComponent* physics = GetScene()->GetComponent<PhysicsComponent>();

    if (!physics) {
      spdlog::error("Failed to retrieve `PhysicsComponent` when trying to destruct `PhysicsObject` did it get destructed earlier?");
      return;
    }

    if (addedToWorld) {
      physics->GetBodyInterface().RemoveBody(bodyId);
    }
    physics->GetBodyInterface().DestroyBody(bodyId);
  }
}

void PhysicsObject::SetShape(JPH::ShapeRefC shape) {
      if (!bodyCreated) {
        spdlog::warn("Tried setting the shape of a body that hasn't been created yet");
        return;
      }
      
      PhysicsComponent* physics = GetScene()->GetComponent<PhysicsComponent>();
      if (!physics) {
        spdlog::warn("Tried setting the shape of a body without a `PhysicsComponent`");
      }

      physics->GetBodyInterface().SetShape(bodyId, shape, true, JPH::EActivation::Activate);
}

void PhysicsObject::Awake() {
  PhysicsComponent* physics = GetScene()->GetComponent<PhysicsComponent>();
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

  bodyCreationSettings.mUserData = reinterpret_cast<JPH::uint64>(this);

  JPH::Body* body = physics->GetBodyInterface().CreateBody(bodyCreationSettings);
  if (!body) {
    spdlog::error("Failed to create a Jolt body");
    bodyCreated = false;
    return;
  }
  bodyId = body->GetID();
  bodyCreated = !bodyId.IsInvalid();
}

void PhysicsObject::OnEnable() {
  spdlog::info("Enable called");
  if (!bodyCreated) {
    spdlog::warn("Tried enabling a body that hasn't been created yet");
    return;
  }

  PhysicsComponent* physics = GetScene()->GetComponent<PhysicsComponent>();
  physics->GetBodyInterface().AddBody(bodyId, JPH::EActivation::Activate);
  addedToWorld = true;
}

void PhysicsObject::OnDisable() {
  if (!bodyCreated) {
    spdlog::warn("Tried disabling a body that hasn't been created yet");
    return;
  }

  PhysicsComponent* physics = GetScene()->GetComponent<PhysicsComponent>();
  if (physics) {
    physics->GetBodyInterface().RemoveBody(bodyId);
    addedToWorld = false;
  }
}
