#include "panels/MainMenuBar.h"
#include "Settings.h"

#include <imgui.h>

namespace Editor {
void MainMenuBar::Draw(bool& shouldClose, Settings& settings) {
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
                    // maybe make it a function so it saves there by itself
                    settings.darkThemeEnabled = true;
                    settings.Save();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                    settings.darkThemeEnabled = false;
                    settings.Save();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace Editor
