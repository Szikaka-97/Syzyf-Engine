#include "ui/UiVisual.h"
#include "imgui.h"

UiVisual::UiVisual(glm::vec4 color, Texture2D* texture) : color(color), texture(texture) {}

void UiVisual::DrawImGui() {
    ImGui::ColorEdit4("Color", &this->color[0]);
}
