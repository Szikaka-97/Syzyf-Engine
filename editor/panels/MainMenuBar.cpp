#include "panels/MainMenuBar.h"
#include "CommandHistory.h"
#include "EditorApplication.h"
#include "FileDialogHelpers.h"
#include "Themes.h"

#include <SDL3/SDL_dialog.h>
#include <imgui.h>

namespace Editor {

void MainMenuBar::Draw(Context& context, bool& shouldClose,
                       Settings& settings) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Scene")) {
                OpenLoadSceneDialog(context);
            }

            if (ImGui::MenuItem("Save Scene As...", nullptr, false,
                                context.selectedScene != nullptr)) {
                OpenSaveSceneDialog(context);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                shouldClose = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (context.selectedScene != nullptr) {
                CommandHistory& commandHistory =
                    context.GetCommandHistory(context.selectedScene);

                if (commandHistory.CanUndo()) {
                    if (ImGui::MenuItem("Undo")) {
                        commandHistory.Undo();
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("Undo");
                    ImGui::EndDisabled();
                }
                if (commandHistory.CanRedo()) {
                    if (ImGui::MenuItem("Redo")) {
                        commandHistory.Redo();
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("Redo");
                    ImGui::EndDisabled();
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::MenuItem("Undo");
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
                    settings.theme = Themes::Theme::Dark;
                    settings.Save();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                    settings.theme = Themes::Theme::Light;
                    settings.Save();
                }
                if (ImGui::MenuItem("Dark Purple")) {
                    Themes::SetDarkPurpleTheme();
                    settings.theme = Themes::Theme::PurpleDark;
                    settings.Save();
                }
                if (ImGui::MenuItem("Light Purple")) {
                    Themes::SetLightPurpleTheme();
                    settings.theme = Themes::Theme::PurpleLight;
                    settings.Save();
                }
                if (ImGui::MenuItem("Classic")) {
                    ImGui::StyleColorsClassic();
                    settings.theme = Themes::Theme::Classic;
                    settings.Save();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Reset to defaults")) {
                settings = Settings{};

                SDL_RestoreWindow(context.window);
                SDL_SetWindowSize(context.window, settings.windowWidth,
                                  settings.windowHeight);
                SDL_SetWindowPosition(context.window, SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED);

                Themes::SetTheme(settings.theme);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace Editor
