#include "scatter/modifiers/ArrayModifier.h"
#include "scatter/Spawner.h"

#include <imgui.h>

namespace Scatter {

ArrayModifier::ArrayModifier(ArraySettings settings) : settings(settings) {}

InstanceStream ArrayModifier::Process(const InstanceStream& input) {
    InstanceStream expandedInstances;
    expandedInstances.reserve(input.size() * (1 + this->settings.arraySize));

    for (const auto& instance : input) {
        expandedInstances.push_back(instance);
        for (int i = 0; i < this->settings.arraySize; i++) {
            expandedInstances.push_back({ glm::translate(instance.transform, this->settings.arrayOffset * glm::vec3((1 + i))) });
            }
        }
    return expandedInstances;
}

void ArraySettings::DrawImGui() {
    ImGui::PushID(this);

    if (ImGui::CollapsingHeader("Array Modifier", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputInt("Array Size", &this->arraySize);
        ImGui::InputFloat3("Array Offset", &this->arrayOffset.x);
    }

    ImGui::PopID();
}
};

