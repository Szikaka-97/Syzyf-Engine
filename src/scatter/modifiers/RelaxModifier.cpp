#include "scatter/modifiers/RelaxModifier.h"

#include <imgui.h>

namespace Scatter {

RelaxModifier::RelaxModifier(RelaxSettings settings) : settings(settings) {}

PointStream RelaxModifier::Process(const PointStream& input) {
    PointStream validPositions;
    validPositions.reserve(input.size());

    for (const glm::vec3& position : input) {
        bool isOverlapping = false;
        for (const glm::vec3& exisitingPosition : validPositions) {
            if (glm::distance(position, exisitingPosition) < this->settings.minDistance) {
            isOverlapping = true;
            break;
            }
        }

        if (!isOverlapping) {
            validPositions.push_back(position);
        }
    }
    return validPositions;
}

void RelaxSettings::DrawImGui() {
    ImGui::InputFloat("Min Distance", &this->minDistance);
    ImGui::InputInt("Max Attempts", &this->maxAttempts);
};
}
