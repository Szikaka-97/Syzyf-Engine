#include "panels/MainMenuBar.h"
#include "Application.h"
#include "Settings.h"

#include <imgui.h>

namespace Editor {
void MainMenuBar::Draw(Context& context, bool& shouldClose,
                       Settings& settings) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                shouldClose = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (context.commandHistory.CanUndo()) {
                if (ImGui::MenuItem("Undo")) {
                    context.commandHistory.Undo();
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Undo");
                ImGui::EndDisabled();
            }
            if (context.commandHistory.CanRedo()) {
                if (ImGui::MenuItem("Redo")) {
                    context.commandHistory.Redo();
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Redo");
                ImGui::EndDisabled();
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
