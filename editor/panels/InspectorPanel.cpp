#include "panels/InspectorPanel.h"
#include "Commands.h"
#include "ComponentRegistry.h"
#include "EditorApplication.h"
#include "MousePickingBody.h"

#include <Debug.h>
#include <Scene.h>

#include <cctype>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <string>

namespace Editor {

void InspectorPanel::Draw(Context& context) {
    ImGui::Begin("Inspector");
    if (context.selectedNode != nullptr) {
        std::string name = context.selectedNode->GetName();
        if (name.empty()) {
            ImGui::TextUnformatted(
                std::to_string(context.selectedNode->GetID()).c_str());
        } else {
            ImGui::TextUnformatted(name.c_str());
        }

        bool nodeEnabled = context.selectedNode->IsEnabled();
        ImGui::Checkbox("Enabled", &nodeEnabled);
        context.selectedNode->SetEnabled(nodeEnabled);

        if (ImGui::TreeNodeEx("Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
            const float size = ImGui::CalcTextSize("00").x;

            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 8; x++) {
                    if (x > 0) {
                        ImGui::SameLine();
                    }

                    std::uint8_t layer = y * 8 + x;

                    ImGui::PushID(layer);

                    if (ImGui::Selectable(std::to_string(layer).c_str(),
                                          context.selectedNode->GetLayer() ==
                                              layer,
                                          0, ImVec2(size, size))) {
                        context.selectedNode->SetLayer(layer);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Position");
            glm::vec3 position =
                context.selectedNode->GlobalTransform().Position();

            ImGui::InputFloat3("##Position", &position[0]);
            if (ImGui::IsItemActivated()) {
                this->initialPosition =
                    context.selectedNode->GlobalTransform().Position();
            }
            if (ImGui::IsItemEdited()) {
                context.selectedNode->GlobalTransform().Position() = position;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<TranslateCommand>(
                        context.selectedNode, this->initialPosition, position));
            }

            glm::vec3 positionDelta = glm::zero<glm::vec3>();
            ImGui::SliderFloat3("##PositionDelta", &positionDelta[0], -1, 1);
            if (ImGui::IsItemActivated()) {
                this->initialPosition =
                    context.selectedNode->GlobalTransform().Position();
            }
            if (ImGui::IsItemEdited()) {
                position += positionDelta;
                context.selectedNode->GlobalTransform().Position() = position;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<TranslateCommand>(
                        context.selectedNode, this->initialPosition,
                        context.selectedNode->GlobalTransform().Position()));
            }

            ImGui::Text("Rotation");
            glm::vec3 rotationEuler = glm::degrees(glm::eulerAngles(
                context.selectedNode->GlobalTransform().Rotation().Value()));

            ImGui::InputFloat3("##Rotation", &rotationEuler[0]);

            if (ImGui::IsItemActivated()) {
                this->initialRotation =
                    context.selectedNode->GlobalTransform().Rotation().Value();
            }
            if (ImGui::IsItemEdited()) {
                context.selectedNode->GlobalTransform().Rotation() =
                    glm::quat(glm::radians(rotationEuler));
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<RotateCommand>(
                        context.selectedNode, initialRotation,
                        context.selectedNode->GlobalTransform()
                            .Rotation()
                            .Value()));
            }

            glm::vec3 rotationDelta = glm::zero<glm::vec3>();
            ImGui::SliderFloat3("##RotationDelta", &rotationDelta[0], -1, 1);
            if (ImGui::IsItemActivated()) {
                this->initialRotation =
                    context.selectedNode->GlobalTransform().Rotation().Value();
            }
            if (ImGui::IsItemEdited()) {
                context.selectedNode->GlobalTransform().Rotation() *=
                    glm::angleAxis(glm::radians(rotationDelta.x),
                                   glm::vec3(1, 0, 0)) *
                    glm::angleAxis(glm::radians(rotationDelta.y),
                                   glm::vec3(0, 1, 0)) *
                    glm::angleAxis(glm::radians(rotationDelta.z),
                                   glm::vec3(0, 0, 1));
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<RotateCommand>(
                        context.selectedNode, this->initialRotation,
                        context.selectedNode->GlobalTransform()
                            .Rotation()
                            .Value()));
            }

            ImGui::Text("Scale");
            glm::vec3 scale = context.selectedNode->GlobalTransform().Scale();

            ImGui::InputFloat3("##Scale", &scale[0]);
            if (ImGui::IsItemActivated()) {
                this->initialScale =
                    context.selectedNode->GlobalTransform().Scale();
            }
            if (ImGui::IsItemEdited()) {
                context.selectedNode->GlobalTransform().Scale() = scale;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<ScaleCommand>(
                        context.selectedNode, this->initialScale,
                        context.selectedNode->GlobalTransform()
                            .Scale()
                            .Value()));
            }

            glm::vec3 scaleDelta = glm::zero<glm::vec3>();
            ImGui::SliderFloat3("##ScaleDelta", &scaleDelta[0], -1, 1);

            if (ImGui::IsItemActivated()) {
                this->initialScale =
                    context.selectedNode->GlobalTransform().Scale();
            }
            if (ImGui::IsItemEdited()) {
                scale += scaleDelta;

                if (glm::abs(scale.x) < 0.0001) {
                    scale.x = 0.0001;
                }
                if (glm::abs(scale.y) < 0.0001) {
                    scale.y = 0.0001;
                }
                if (glm::abs(scale.z) < 0.0001) {
                    scale.z = 0.0001;
                }
                context.selectedNode->GlobalTransform().Scale() = scale;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                context.commandHistory.ExecuteCommand(
                    std::make_unique<ScaleCommand>(
                        context.selectedNode, this->initialScale,
                        context.selectedNode->GlobalTransform()
                            .Scale()
                            .Value()));
            }

            ImGui::TreePop();
        }

        int index = 0;
        for (GameObject* obj : context.selectedNode->AttachedObjects()) {
            if (dynamic_cast<MousePickingBody*>(obj) != nullptr) {
                continue;
            }

            ImGui::PushID(obj->GetID());
            if (ImGui::TreeNode(
                    std::format("{}: {}", index, obj->GetName()).c_str())) {
                ImGui::Text("Object ID: %i", obj->GetID());

                bool objEnabled = obj->IsEnabled();

                ImGui::Checkbox("Enabled", &objEnabled);

                obj->SetEnabled(objEnabled);

                ImGuiDrawable* imguiObj = dynamic_cast<ImGuiDrawable*>(obj);

                if (imguiObj) {
                    ImGui::Separator();

                    imguiObj->DrawImGui();
                }
                ImGui::TreePop();
            }
            index++;
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
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
                        context.selectedNode->GetName().c_str());
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

            ComponentFactoryFunc firstMatchFunction = nullptr;
            bool isFirstMatch = true;

            for (const auto& [name, factoryFunc] :
                 ComponentRegistry::Get().GetFactories()) {
                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(),
                               lowerName.begin(), ::tolower);

                if (searchString.empty() ||
                    lowerName.find(searchString) != std::string::npos) {
                    if (isFirstMatch) {
                        firstMatchFunction = factoryFunc;
                    }

                    if (ImGui::Selectable(name.c_str(), isFirstMatch)) {
                        factoryFunc(context.selectedNode);
                        ImGui::CloseCurrentPopup();
                    }

                    isFirstMatch = false;
                }
            }

            if (enterPressed && firstMatchFunction != nullptr) {
                firstMatchFunction(context.selectedNode);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
} // namespace Editor
