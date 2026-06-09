#pragma once

#include "Texture.h"

#include <cstdint>
#include <imgui.h>
#include <string>
#include <functional>

class Texture2D;

inline bool DrawTextureField(const char* label, Texture2D* currentTexture, ImVec2 size, const std::function<void(const std::string&)>& onFileSelected) {
    bool updated = false;

    ImGui::PushID(label);

    if (currentTexture != nullptr) {
        ImGui::ImageButton("##preview", (void*)(intptr_t)currentTexture->GetHandle(), size);
    } else {
        ImGui::Button("Drop", size);
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FILE_PATH")) {
            std::string path = (const char*)payload->Data;

            onFileSelected(path);
            updated = true;
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::Text("%s", label);

    ImGui::PopID();

    return updated;
}
