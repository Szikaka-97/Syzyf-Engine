#include "panels/CommandHistoryPanel.h"

#include "Application.h"
#include "imgui.h"

namespace Editor {
void CommandHistoryPanel::Draw(Context& context) {
    ImGui::Begin("Command History");

    const auto& history = context.commandHistory.GetHistorY();
    int currentIndex = context.commandHistory.GetCurrentIndex();

    if (history.empty()) {
        ImGui::TextDisabled("No history");
        ImGui::End();
        return;
    }

    for (int i = 0; i < static_cast<int>(history.size()); ++i) {
        bool isApplied = (i <= currentIndex);
        bool isCurrent = (i == currentIndex);

        if (!isApplied) {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        std::string label = history[i]->GetName() + "##" + std::to_string(i);

        if (ImGui::Selectable(label.c_str(), isCurrent)) {
            if (i < currentIndex) {
                while (context.commandHistory.GetCurrentIndex() > i) {
                    context.commandHistory.Undo();
                }
            } else if (i > currentIndex) {
                while (context.commandHistory.GetCurrentIndex() < i) {
                    context.commandHistory.Redo();
                }
            }
        }

        if (!isApplied) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();
}
} // namespace Editor
