#include "Surface.h"
#include "Mesh.h"
#include "Scene.h"
#include "physics/System.h"
#include <random>
#include <limits>
#include <spdlog/spdlog.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

Surface::Surface(Mesh* floorMesh, float cellSize)
    : floorMesh(floorMesh), cellSize(cellSize) {
    if (!floorMesh || floorMesh->GetSubMeshCount() == 0) {
       // spdlog::error("Surface: No valid mesh provided or mesh has no submeshes");
        return;
    }

    SceneNode* node = GetNode();
    if (!node) {
       // spdlog::error("Surface: Node not found");
        return;
    }

    CollectVertices();
    spdlog::info("Surface generated {} walkable points", walkablePoints.size());
}

Surface::~Surface() {}

void Surface::CollectVertices() {
    walkablePoints.clear();
    if (!floorMesh) return;

    unsigned int vertexCount = floorMesh->GetVertexCount();
    const float* vertexData = floorMesh->GetVertexData();
    unsigned int stride = floorMesh->GetVertexStride();

    SceneNode* node = GetNode();
    glm::mat4 world = node->GlobalTransform();

    for (unsigned int i = 0; i < vertexCount; ++i) {
        const float* v = vertexData + i * stride;
        glm::vec3 localPos(v[0], v[1], v[2]);
        glm::vec3 worldPos = world * glm::vec4(localPos, 1.0f);
        walkablePoints.push_back(worldPos);
    }
}

glm::vec3 Surface::GetRandomWalkPoint(const glm::vec3& center, float radius) const {
    if (walkablePoints.empty()) {
        spdlog::error("No walkable points found on surface, returning center point: ({}, {}, {})", center.x, center.y, center.z);
        return center;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, walkablePoints.size() - 1);

    /*for (int attempts = 0; attempts < 20; ++attempts) {
        const auto& candidate = walkablePoints[dist(gen)];
        if (glm::distance(candidate, center) <= radius) {
            return candidate;
        }
    }

    spdlog::warn("No walk point in radius, picking random point from whole list");*/
    return walkablePoints[dist(gen)];
}

float Surface::GetGroundHeight(float x, float z) const {
    auto* physics = GetScene()->GetComponent<Physics::System>();
    if (!physics) return 0.0f;

    JPH::RRayCast ray(JPH::RVec3(x, 500.0f, z), JPH::Vec3(0, -1, 0));
    JPH::RayCastResult result;
    if (physics->GetSystem().GetNarrowPhaseQuery().CastRay(ray, result)) {
        JPH::RVec3 hit = ray.GetPointOnRay(result.mFraction);
        return static_cast<float>(hit.GetY());
    }
    return 0.0f;
}

bool Surface::IsOnSurface(const glm::vec3& point) const {
    auto* physics = GetScene()->GetComponent<Physics::System>();
    if (!physics) return false;

    JPH::RRayCast ray(JPH::RVec3(point.x, point.y, point.z), JPH::Vec3(0, -1, 0));
    JPH::RayCastResult result;
    if (physics->GetSystem().GetNarrowPhaseQuery().CastRay(ray, result)) {
        return true;
    }
    return false;
}