#include "scatter/modifiers/TransformModifier.h"
#include "scatter/Spawner.h"

#include <glm/gtc/random.hpp>

namespace Scatter {

TransformModifier::TransformModifier(TransformSettings settings) : settings(settings) {}

InstanceStream TransformModifier::Process(const PointStream& input) {
    InstanceStream output;
    output.reserve(input.size());
    
    for (const glm::vec3& position : input) {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        glm::vec3 randomRotation = glm::linearRand(settings.minRotation, settings.maxRotation);

        transform = glm::rotate(transform, randomRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, randomRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        float randomScale = glm::linearRand(settings.minScale, settings.maxScale);
        transform = glm::scale(transform, glm::vec3(randomScale));

        output.push_back({ transform });
    }
    return output;
}

void TransformSettings::DrawImGui() {
    ImGui::PushID(this);

    if (ImGui::CollapsingHeader("Transform Modifier", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputFloat("Min Scale", &this->minScale);
        ImGui::InputFloat("Max Scale", &this->maxScale);
        ImGui::InputFloat3("Min Rotation", &this->minRotation.x);
        ImGui::InputFloat3("Max Rotation", &this->maxRotation.x);
    }

    ImGui::PopID();
}
}
