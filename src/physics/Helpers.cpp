#include "physics/Helpers.h"

#include "Mesh.h"
#include "MeshRenderer.h"

#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

namespace Physics {

const float DEFAULT_CONVEX_RADIUS = 0.05f;

JPH::ShapeRefC SphereShape(float radius) {
  // If the radius is to small it complains about not being able to calculate the mass and does a SIGTRAP
  if (radius < 0.001f) {
    spdlog::warn("Trying to create a `PhysicsObjet::Sphere` with too small of a radius, setting it to 0.001");
    radius = 0.001f; 
  }

  return new JPH::SphereShape(radius);
}

JPH::ShapeRefC BoxShape(glm::vec3 halfExtent) {
  if (halfExtent.x < DEFAULT_CONVEX_RADIUS || halfExtent.y < DEFAULT_CONVEX_RADIUS || halfExtent.z < DEFAULT_CONVEX_RADIUS) {
    spdlog::warn("Trying to create a `PhysicsObject::Box` with extents smaller than Jolt's default convex radius. Clamping to 0.05f");
    halfExtent.x = std::max(halfExtent.x, DEFAULT_CONVEX_RADIUS);
    halfExtent.y = std::max(halfExtent.y, DEFAULT_CONVEX_RADIUS);
    halfExtent.z = std::max(halfExtent.z, DEFAULT_CONVEX_RADIUS);
  }
  
    return new JPH::BoxShape(JPH::Vec3Arg(halfExtent.x, halfExtent.y, halfExtent.z));
}

JPH::ShapeRefC CapsuleShape(float halfHeight, float radius) {
  if (halfHeight < DEFAULT_CONVEX_RADIUS || radius < DEFAULT_CONVEX_RADIUS) {
    spdlog::warn("Trying to create a `PhysicsObject::Capsule` with dimensions smaller than Jolt's convex radius. Clamping to 0.05f");
    halfHeight = std::max(halfHeight, DEFAULT_CONVEX_RADIUS);
    radius = std::max(halfHeight, DEFAULT_CONVEX_RADIUS);
  }

    return new JPH::CapsuleShape(halfHeight, radius);
}

JPH::ShapeRefC PlaneShape(glm::vec3 normal) {
  JPH::Vec3 joltNormal(normal.x, normal.y, normal.z);
  joltNormal = joltNormal.Normalized();

  return new JPH::PlaneShape(JPH::Plane(joltNormal, 0.0f));
}

JPH::ShapeRefC ConvexHullMeshShape(const Mesh* mesh) {
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

  JPH::ConvexHullShapeSettings shapeSettings = JPH::ConvexHullShapeSettings(
    joltVertices.data(),
    joltVertices.size()
  );
  shapeSettings.mMaxConvexRadius = Physics::DEFAULT_CONVEX_RADIUS;
  shapeSettings.mHullTolerance = 0.05f;

  JPH::Shape::ShapeResult result = shapeSettings.Create();

  if (result.IsValid()) {
      return result.Get();
  } else {
    spdlog::error("Physics::ConvexHullMesh: Failed to create a convex hull mesh");
    return nullptr;
  }
}

JPH::ShapeRefC MeshShape(const Mesh* mesh) {
  const uint8_t* vertexDataPointer = reinterpret_cast<const uint8_t*>(mesh->GetVertexData());
  const unsigned int vertexStride = mesh->GetVertexStride() * sizeof(float);

  JPH::TriangleList triangles;

  for (const auto& subMesh : mesh->GetSubMeshes()) {
    if (subMesh.GetType() != Mesh::MeshType::Triangles) {
      continue;
    }

    const unsigned int* indices = subMesh.GetIndexData();
    unsigned int faceCount = subMesh.GetFaceCount();

    for (unsigned int i = 0; i < faceCount * 3; i += 3) {
      const float* p1 = reinterpret_cast<const float*>(vertexDataPointer + indices[i] * vertexStride);
      JPH::Vec3 v1(p1[0], p1[1], p1[2]);

      const float* p2 = reinterpret_cast<const float*>(vertexDataPointer + indices[i + 1] * vertexStride);
      JPH::Vec3 v2(p2[0], p2[1], p2[2]);

      const float* p3 = reinterpret_cast<const float*>(vertexDataPointer + indices[i + 2] * vertexStride);
      JPH::Vec3 v3(p3[0], p3[1], p3[2]);

      triangles.emplace_back(v1, v2, v3);
    }
  }

  if (triangles.empty()) {
    spdlog::error("Physics::Body::Mesh: Mesh has no valid triangles, using a 0.1f sphere as fallback");
    return SphereShape(0.1f);
  }

  JPH::MeshShapeSettings shapeSettings = JPH::MeshShapeSettings(triangles);
  shapeSettings.Sanitize();

  JPH::Shape::ShapeResult result = shapeSettings.Create();

  if (result.IsValid()) {
      return result.Get();
  } else {
      spdlog::error("Physics::ConvexHullMesh: Failed to create a convex hull mesh, using a 0.1f sphere as fallback");
      return SphereShape(0.1f);
  }
}

JPH::ShapeRefC CreateCompoundShapeFromNode(SceneNode* rootNode, bool useConvex, JPH::EMotionType motionType, JPH::ObjectLayer layer) {
    JPH::StaticCompoundShapeSettings compoundSettings;

    auto traverse = [&](auto& self, SceneNode* node) -> void {
        if (MeshRenderer* renderer = node->GetObject<MeshRenderer>()) {
            if (Mesh* mesh = renderer->GetMesh()) {
                JPH::ShapeRefC shape;
                if (useConvex) {
                    shape = ConvexHullMeshShape(mesh);
                } else {
                    shape = MeshShape(mesh);
                }

                if (!shape) {
                    spdlog::warn("Physics::CreateCompoundShapeFromNode: Failed to create a sub collision shape");
                    return;
                }

                glm::mat4 rootGlobal = rootNode->GetTransform().GlobalTransform().Value();
                glm::mat4 nodeGlobal = node->GetTransform().GlobalTransform().Value();
                glm::mat4 relativeMatrix = glm::inverse(rootGlobal) * nodeGlobal;


                glm::vec3 position = relativeMatrix[3];

                glm::vec3 scale(
                    glm::length(glm::vec3(relativeMatrix[0])),
                    glm::length(glm::vec3(relativeMatrix[1])),
                    glm::length(glm::vec3(relativeMatrix[2]))
                );

                glm::mat3 rotMat(
                    glm::vec3(relativeMatrix[0]) / scale.x,
                    glm::vec3(relativeMatrix[1]) / scale.y,
                    glm::vec3(relativeMatrix[2]) / scale.z
                );
                glm::quat rotation = glm::quat_cast(rotMat);

                JPH::Vec3 jphPosition(position.x, position.y, position.z);
                JPH::Quat jphRotation = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w).Normalized();

                JPH::ShapeRefC finalShape = shape;
                if (glm::any(glm::notEqual(scale, glm::vec3(1.0f), 0.0001f))) {
                    finalShape = new JPH::ScaledShape(shape, JPH::Vec3(scale.x, scale.y, scale.z));
                }

                compoundSettings.AddShape(jphPosition, jphRotation, finalShape);
                }
            }
            for (SceneNode* child : node->GetChildren()) {
                self(self, child);
            }
        };
        traverse(traverse, rootNode);

        JPH::ShapeSettings::ShapeResult result = compoundSettings.Create();
        if (result.IsValid()) {
            return result.Get();
        }

        return nullptr;
    }
}
