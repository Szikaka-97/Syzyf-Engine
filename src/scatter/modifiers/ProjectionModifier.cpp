#include "scatter/modifiers/ProjectionModifier.h"
#include "imgui.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <glm/glm.hpp>

namespace Scatter {
ProjectionModifier::ProjectionModifier(ProjectionSettings settings, JPH::PhysicsSystem* joltSystem, glm::mat4 transform, glm::mat4 inverseTransform)
    : settings(settings), joltSystem(joltSystem), transform(transform), inverseTransform(inverseTransform) {}

PointStream ProjectionModifier::Process(const PointStream& input) {
    PointStream output;
    output.reserve(input.size());

    for (const glm::vec3& position : input) {
        glm::vec3 glmOrigin = this->transform * glm::vec4(position + -settings.raycastDirection * settings.raycastOffset, 1.0f);
        glm::vec3 glmDirection = glm::normalize(settings.raycastDirection) * settings.raycastLength;

        JPH::RVec3 origin(glmOrigin.x, glmOrigin.y, glmOrigin.z);
        JPH::RVec3 direction(glmDirection.x, glmDirection.y, glmDirection.z);
        JPH::RRayCast ray(origin, direction);

        JPH::RayCastResult hit;
        if (joltSystem->GetNarrowPhaseQuery().CastRay(ray, hit)) {
            output.push_back(glm::vec3(this->inverseTransform * glm::vec4(glmOrigin + (glmDirection * hit.mFraction), 1.0f)));
        }
    }
    return output;
}

void ProjectionModifier::DrawImGui() {
    ImGui::PushID(this);

    if (ImGui::CollapsingHeader("Projection Modifier", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputFloat("Raycast Length", &this->settings.raycastLength);
        ImGui::InputFloat("Raycast Offset", &this->settings.raycastOffset);
        ImGui::InputFloat3("Ray Direction", &this->settings.raycastDirection.x);
    }

    ImGui::PopID();
}
}
