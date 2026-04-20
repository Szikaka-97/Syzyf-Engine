#pragma once

#include "scatter/modifiers/IModifiers.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <imgui.h>

namespace Scatter {

struct ProjectionSettings {
public:
    float raycastLength = 5.0f;
    float raycastOffset = 5.0f;
    glm::vec3 raycastDirection = { 0.0f, -1.0f, 0.0f };
public:
    void DrawImGui();
};

class ProjectionModifier : public IPointModifier {
private:
    ProjectionSettings settings;
    JPH::PhysicsSystem* joltSystem;
    glm::mat4 transform;
    glm::mat4 inverseTransform;
public:
    ProjectionModifier(ProjectionSettings settings, JPH::PhysicsSystem* joltSystem, glm::mat4 transform, glm::mat4 inverseTransform);

    PointStream Process(const PointStream& input);
    
    void DrawImGui();
};
}
