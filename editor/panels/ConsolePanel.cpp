#include "panels/ConsolePanel.h"
#include <mutex>

namespace Editor {

void ConsolePanel::Draw() {
    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        ImGuiConsoleSink<std::mutex>::logs.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-Scroll", &ImGuiConsoleSink<std::mutex>::autoScroll);

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    for (const std::string& log : ImGuiConsoleSink<std::mutex>::logs) {
        ImGui::TextUnformatted(log.c_str());
    }

    if (ImGuiConsoleSink<std::mutex>::autoScroll &&
        ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
} // namespace Editor
