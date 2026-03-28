#include "Surface.h"
#include "Mesh.h"
#include "Scene.h"
#include "physics/System.h"
#include <random>
#include <limits>

#include <Jolt/Physics/Collision/RayCast.h>   
#include <Jolt/Physics/Collision/CastResult.h>

Surface::Surface(Mesh* floorMesh, float cellSize)
    : floorMesh(floorMesh), cellSize(cellSize) {
    if (floorMesh && floorMesh->GetSubMeshCount() > 0) {
        BoundingBox localBounds = floorMesh->SubMeshAt(0).GetBounds();
        glm::vec3 localCenter = localBounds.GetCenter();
        glm::vec3 localExtents = localBounds.GetExtents();
        glm::vec3 localMin = localCenter - localExtents;
        glm::vec3 localMax = localCenter + localExtents;

        glm::mat4 world = this->GlobalTransform();
        glm::vec3 worldMin = world * glm::vec4(localMin, 1.0f);
        glm::vec3 worldMax = world * glm::vec4(localMax, 1.0f);

        GenerateGrid(worldMin.x, worldMax.x, worldMin.z, worldMax.z);
    }
}

Surface::~Surface() {}

void Surface::GenerateGrid(float minX, float maxX, float minZ, float maxZ) {
    walkablePoints.clear();
    for (float x = minX; x <= maxX; x += cellSize) {
        for (float z = minZ; z <= maxZ; z += cellSize) {
            glm::vec3 point(x, 100.0f, z); 
            if (IsOnSurface(point)) {
                walkablePoints.push_back(point);
            }
        }
    }
}

glm::vec3 Surface::GetRandomWalkPoint(const glm::vec3& center, float radius) const {
    if (walkablePoints.empty()) return center;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, walkablePoints.size() - 1);

    for (int attempts = 0; attempts < 10; ++attempts) {
        const auto& candidate = walkablePoints[dist(gen)];
        if (glm::distance(candidate, center) <= radius) {
            return candidate;
        }
    }

    // if failed return the closest walkable point within radius
    size_t closestIdx = 0;
    float minDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < walkablePoints.size(); ++i) {
        float d = glm::distance(walkablePoints[i], center);
        if (d < minDist && d <= radius) {
            minDist = d;
            closestIdx = i;
        }
    }
    return walkablePoints[closestIdx];
}

float Surface::GetGroundHeight(float x, float z) const {
    auto* physics = GetScene()->GetComponent<Physics::System>();
    if (!physics) return 0.0f;

    JPH::RRayCast ray(JPH::RVec3(x, 100.0f, z), JPH::Vec3(0, -1, 0));
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
        JPH::RVec3 hit = ray.GetPointOnRay(result.mFraction);
        // to do: check normal angle to avoid steep slopes
        return true;
    }
    return false;
}