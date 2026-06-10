#include "panels/SystemsDebugPanel.h"
#include "EditorApplication.h"
#include "Graphics.h"
#include "Profiler.h"
#include <nlohmann/json.hpp>

#include <Scene.h>
#include <algorithm>
#include <imgui.h>
#include <physics/System.h>

SceneComponent* MessagingHelpers_AddComponentToScene(Scene* scene, const std::string& objectName);
std::vector<std::string> MessagingHelpers_GetAvailableComponents(); // I love how sketchy it is

void DrawProfilerInfo(nlohmann::json node, std::string name) {
    if (node.size() > 1) {
        if (ImGui::TreeNode(name.c_str(), "%s: %.03fms", name.c_str(), node["_value"].get<float>() * 1000)) {
            for (auto child : node.items()) {
                if (child.value().is_object()) {
                    DrawProfilerInfo(child.value(), child.key());
                }
            }

            ImGui::TreePop();
        }
    }
    else {
        ImGui::Text("%s: %.03fms", name.c_str(), node["_value"].get<float>() * 1000);
    }
}

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

        nlohmann::json profilerRoot;

        for (auto& node : results) {
            nlohmann::json* jsonNode = &profilerRoot;

            for (auto piece : node.name) {
                if (jsonNode->contains(piece.string())) {
                    jsonNode = &(*jsonNode)[piece.string()];
                }
                else {
                    jsonNode = &((*jsonNode)[piece.string()] = json{});
                }
            }

            (*jsonNode)["_value"] = node.time;
        }

        DrawProfilerInfo(profilerRoot["Frame"], "Frame");

        ImGui::TreePop();

		Profiler::Step();
	}

    ImGui::End();
}
} // namespace Editor
