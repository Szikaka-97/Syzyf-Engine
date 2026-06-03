#include "Popups.h"

#include <imgui.h>

namespace Editor {
bool DrawRenameModal(const char* popupId, const std::string& targetName,
                     char* nameBuffer, size_t bufferSize) {
    bool confirmed = false;

    if (ImGui::BeginPopupModal(popupId, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Rename: %s", targetName.c_str());

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        bool enterPressed =
            ImGui::InputText("##NewName", nameBuffer, bufferSize,
                             ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("Save", ImVec2(120, 0)) || enterPressed) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return confirmed;
}

bool DrawDeleteModal(const char* popupId, const std::string& targetName) {
    bool confirmed = false;

    if (ImGui::BeginPopupModal(popupId, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                           "Are you sure you want to delete '%s'?",
                           targetName.c_str());
        ImGui::Separator();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return confirmed;
}
} // namespace Editor
