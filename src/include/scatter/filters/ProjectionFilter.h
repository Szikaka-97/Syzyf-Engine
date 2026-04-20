#pragma once

#include "scatter/filters/IFilters.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Scatter {

struct ProjectionSettings {
    float raycastLength = 5.0f;
    float raycastOffset = 5.0f;
    glm::vec3 raycastDirection = { 0.0f, -1.0f, 0.0f };
};

class ProjectionFilter : public IPointFilter {
private:
    ProjectionSettings settings;
    JPH::PhysicsSystem* joltSystem;
    glm::mat4 transform;
    glm::mat4 inverseTransform;
public:
    ProjectionFilter(ProjectionSettings settings, JPH::PhysicsSystem* joltSystem, glm::mat4 transform, glm::mat4 inverseTransform);

    PointStream Process(const PointStream& input);
};
}
