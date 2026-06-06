#include "panels/SystemsDebugPanel.h"
#include "EditorApplication.h"
#include "Graphics.h"
#include "Profiler.h"

#include <Scene.h>
#include <algorithm>
#include <imgui.h>
#include <physics/System.h>

SceneComponent* MessagingHelpers_AddComponentToScene(Scene* scene, const std::string& objectName);
std::vector<std::string> MessagingHelpers_GetAvailableComponents(); // I love how sketchy it is

namespace Editor {
void SystemsDebugPanel::Draw(Context& context) {
    ImGui::Begin("Systems");

    if (context.selectedScene) {
        context.selectedScene->DrawImGui();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add Scene Component", ImVec2(-1, 30))) {
            ImGui::OpenPopup("AddComponentPopup");
            this->focusComponentSearch = true;
            this->componentSearchBuffer[0] = '\0';
        }
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing,
                                ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopup("AddComponentPopup",
                              ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select a component to add to: %s",
                        context.selectedScene->name.c_str());
            ImGui::Separator();

            if (this->focusComponentSearch) {
                ImGui::SetKeyboardFocusHere();
                this->focusComponentSearch = false;
            }

            bool enterPressed = ImGui::InputTextWithHint(
                "##Search", "Search...", this->componentSearchBuffer,
                sizeof(this->componentSearchBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue |
                    ImGuiInputTextFlags_AutoSelectAll);

            ImGui::Separator();

            std::string searchString = this->componentSearchBuffer;
            std::transform(searchString.begin(), searchString.end(),
                           searchString.begin(), ::tolower);

            bool isFirstMatch = true;
            std::string firstMatchComponent = "";

            std::vector availableComponents = MessagingHelpers_GetAvailableComponents();

            for (const auto& name : availableComponents) {
                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(),
                               lowerName.begin(), ::tolower);

                if (searchString.empty() ||
                    lowerName.find(searchString) != std::string::npos) {
                    if (isFirstMatch) {
                        firstMatchComponent = name;
                    }

                    if (ImGui::Selectable(name.c_str(), isFirstMatch)) {
                        MessagingHelpers_AddComponentToScene(context.selectedScene, name);
                        ImGui::CloseCurrentPopup();
                    }

                    isFirstMatch = false;
                }
            }

            if (enterPressed && !firstMatchComponent.empty()) {
                MessagingHelpers_AddComponentToScene(context.selectedScene, firstMatchComponent);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

	if (ImGui::TreeNode("Profiler")) {
		std::vector<Profiler::ProfilerResult> results = Profiler::GrabResults();

		int slashCount = -1;
		bool levelOpen = true;

		for (const auto& result : results) {
			int newSlashCount = std::count(result.name.begin(), result.name.end(), '/');

			if (newSlashCount <= slashCount && levelOpen) {
				ImGui::TreePop();
			}

			if (newSlashCount == slashCount) {
				levelOpen = true;
			}
		
			if (levelOpen) {
				levelOpen = ImGui::TreeNode(result.name.c_str(), "%s: %.03fms", result.name.c_str(), result.time * 1000);
			}

			slashCount = newSlashCount;
		}

		Profiler::Step();

		ImGui::TreePop();
	}

    ImGui::End();
}
} // namespace Editor
