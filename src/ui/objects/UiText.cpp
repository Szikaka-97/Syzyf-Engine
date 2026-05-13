#include "ui/objects/UiText.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

UiText::UiText(std::string text, Font* font) : text(std::move(text)), font(font) {}

void UiText::DrawImGui() {
    ImGui::InputTextMultiline("Text", &this->text, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 5)); 

    ImGui::ColorEdit4("Color", &this->color[0]);
    ImGui::InputFloat("Font Size", &this->fontSize);
}
