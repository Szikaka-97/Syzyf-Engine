#include "panels/FilesPanel.h"

#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace Editor {
void FilesPanel::Draw() {
    ImGui::Begin("Files");

    if (this->currentDirectory != "./res/" &&
        this->currentDirectory != "./res") {
        if (ImGui::Button("Back")) {
            this->currentDirectory = this->currentDirectory.parent_path();
        }
        ImGui::Separator();
    }

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / this->CELL_SIZE);
    if (columnCount < 1) {
        columnCount = 1;
    }

    if (ImGui::BeginTable("FileBrowserGrid", columnCount)) {
        if (fs::exists(this->currentDirectory)) {
            for (auto& directoryEntry :
                 fs::directory_iterator(this->currentDirectory)) {
                ImGui::TableNextColumn();

                const auto& path = directoryEntry.path();
                std::string filenameString = path.filename().string();

                ImGui::PushID(filenameString.c_str());

                // Replace with real icons
                std::string iconStr =
                    directoryEntry.is_directory() ? "[ DIR ]" : "[ FILE ]";

                if (ImGui::Button(
                        iconStr.c_str(),
                        ImVec2(this->THUMBNAIL_SIZE, this->THUMBNAIL_SIZE))) {
                    if (directoryEntry.is_directory()) {
                        this->currentDirectory /= path.filename();
                        ImGui::PopID();
                        break;
                    }
                }

                if (!directoryEntry.is_directory()) {
                    std::string fullPath = path.string();

                    if (ImGui::BeginDragDropSource(
                            ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload("DND_FILE_PATH",
                                                  fullPath.c_str(),
                                                  fullPath.size() + 1);

                        ImGui::Text("Drop %s", filenameString.c_str());

                        ImGui::EndDragDropSource();
                    }
                }

                ImGui::TextWrapped("%s", filenameString.c_str());

                ImGui::PopID();
            }
        } else {
            ImGui::Text("Directory %s not found",
                        this->currentDirectory.string().c_str());
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
} // namespace Editor
