#include "ui/UiLayout.h"
#include "imgui.h"

UiLayout::UiLayout(glm::ivec2 size, glm::ivec2 offset, int zIndex, AnchorPoint anchorPoint) : size(size), offset(offset), anchorPoint(anchorPoint) {}

void UiLayout::DrawImGui() {
    ImGui::InputInt2("Offset", &this->offset[0]);
    ImGui::InputInt2("Size", &this->size[0]);

    ImGui::Separator();

    ImGui::Text("Anchor Point");

    DrawAnchorButton("TL", AnchorPoint::TopLeft);
    ImGui::SameLine();
    DrawAnchorButton("TC", AnchorPoint::TopCenter);
    ImGui::SameLine();
    DrawAnchorButton("TR", AnchorPoint::TopRight);

    DrawAnchorButton("CL", AnchorPoint::CenterLeft);
    ImGui::SameLine();
    DrawAnchorButton("C", AnchorPoint::Center);
    ImGui::SameLine();
    DrawAnchorButton("CR", AnchorPoint::CenterRight);

    DrawAnchorButton("BL", AnchorPoint::BottomLeft);
    ImGui::SameLine();
    DrawAnchorButton("BC", AnchorPoint::BottomCenter);
    ImGui::SameLine();
    DrawAnchorButton("BR", AnchorPoint::BottomRight);
}

void UiLayout::DrawAnchorButton(const char* label, AnchorPoint point) {
    bool isSelected = (anchorPoint == point);
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    if (ImGui::Button(label, ImVec2(30, 30))) {
        anchorPoint = point;
    }

    if (isSelected) {
        ImGui::PopStyleColor();
    }
}
