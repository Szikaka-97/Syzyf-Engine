#include "ui/objects/UiVisual.h"
#include <imgui.h>

UiVisual::UiVisual(glm::vec4 color, Texture2D* texture) : color(color), texture(texture) {}

void UiVisual::DrawImGui() {
    ImGui::ColorEdit4("Color", &this->color[0]);
    
    DrawOptionalColor("Hovered Color", this->colorHovered);
    DrawOptionalColor("Clicked Color", this->colorClicked);
    DrawOptionalColor("Disabled Color", this->colorDisabled);
}

void UiVisual::DrawOptionalColor(const char* label, std::optional<glm::vec4>& color) {
    bool isEnabled = color.has_value();

    std::string checkboxLabel = "##Enable" + std::string(label);
    if (ImGui::Checkbox(checkboxLabel.c_str(), &isEnabled)) {
        if (isEnabled) {
            color = glm::vec4(1.0f);
        } else {
            color = std::nullopt;
        }
    }

    ImGui::SameLine();

    if (isEnabled) {
        ImGui::ColorEdit4(label, &color.value()[0]);
    } else {
        ImGui::BeginDisabled();
        glm::vec4 dummyColor = glm::vec4(0.5f);
        ImGui::ColorEdit4(label, &dummyColor[0]);
        ImGui::EndDisabled();
    }
}
