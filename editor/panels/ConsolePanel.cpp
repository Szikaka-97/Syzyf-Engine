#include "panels/ConsolePanel.h"
#include "EditorApplication.h"

#include <mutex>

namespace Editor {

void ConsolePanel::Draw(Context& context) {
    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        ImGuiConsoleSink<std::mutex>::logs.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-Scroll", &ImGuiConsoleSink<std::mutex>::autoScroll);

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushFont(context.consoleFont);

    for (const auto& log : ImGuiConsoleSink<std::mutex>::logs) {
        ImVec4 color;
        bool hasColor = true;

        switch (log.level) {
        case spdlog::level::trace:
        case spdlog::level::debug:
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            break;
        case spdlog::level::info:
            hasColor = false;
            break;
        case spdlog::level::warn:
            color = ImVec4(0.8f, 0.5f, 0.0f, 1.0f);
            break;
        case spdlog::level::err:
        case spdlog::level::critical:
            color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            break;
        default:
            hasColor = false;
            break;
        }

        size_t firstBracket = log.text.find(']');
        size_t splitPos = std::string::npos;

        if (firstBracket != std::string::npos) {
            size_t secondBracket = log.text.find(']', firstBracket + 1);
            if (secondBracket != std::string::npos) {
                splitPos = secondBracket + 1;
                if (splitPos < log.text.size() && log.text[splitPos] == ' ') {
                    splitPos++;
                }
            }
        }

        if (splitPos != std::string::npos) {
            std::string header = log.text.substr(0, splitPos);
            std::string body = log.text.substr(splitPos);

            if (hasColor)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(header.c_str());
            if (hasColor)
                ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(body.c_str());
        } else {
            if (hasColor)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.text.c_str());
            if (hasColor)
                ImGui::PopStyleColor();
        }
    }

    ImGui::PopFont();

    if (ImGuiConsoleSink<std::mutex>::autoScroll &&
        ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
} // namespace Editor
