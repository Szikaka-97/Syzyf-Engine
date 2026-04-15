#include "panels/MainMenuBar.h"

#include <imgui.h>

namespace Editor {
void MainMenuBar::Draw(bool& shouldClose) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                shouldClose = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark")) {
                    ImGui::StyleColorsDark();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace Editor
