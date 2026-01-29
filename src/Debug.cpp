#include <Debug.h>

#include <stack>

#include <imgui.h>

#include <Scene.h>

DebugInspector::DebugInspector(Scene* scene):
SceneComponent(scene) { }

void DebugInspector::DrawImGui() {
	SceneNode* treeRoot = GetScene()->GetRootNode();
	std::stack<SceneNode*> nodes;

	nodes.push_range(treeRoot->GetChildren());

	if (ImGui::TreeNode("GameObject debug")) {
		while (!nodes.empty()) {
			SceneNode* node = nodes.top();
			nodes.pop();
			
			ImGui::PushID(node->GetID());

			std::string treeHeader = node->GetName();

			if (treeHeader.empty()) {
				treeHeader = std::to_string(node->GetID());
			}

			if (ImGui::TreeNode(treeHeader.c_str())) {
				ImGui::Text("Node ID: %i", node->GetID());

				if (ImGui::TreeNode("Transform")) {
					ImGui::Text("Position");

					glm::vec3 position = node->GlobalTransform().Position();
		
					ImGui::InputFloat3("##Position", &position[0]);
	
					glm::vec3 positionDelta = glm::zero<glm::vec3>();
	
					ImGui::SliderFloat3("##PositionDelta", &positionDelta[0], -1, 1);
	
					position += positionDelta;
		
					node->GlobalTransform().Position() = position;

					ImGui::Text("Rotation");

					glm::vec3 rotationEuler = glm::degrees(glm::eulerAngles(node->GlobalTransform().Rotation().Value()));

					ImGui::InputFloat3("##Rotation", &rotationEuler[0]);
	
					node->GlobalTransform().Rotation() = glm::quat(glm::radians(rotationEuler));

					glm::vec3 rotationDelta = glm::zero<glm::vec3>();
	
					ImGui::SliderFloat3("##RotationDelta", &rotationDelta[0], -1, 1);
	
					node->GlobalTransform().Rotation() *= glm::angleAxis(
						glm::radians(rotationDelta.x),
						glm::vec3(1, 0, 0)
					) * glm::angleAxis(
						glm::radians(rotationDelta.y),
						glm::vec3(0, 1, 0)
					) * glm::angleAxis(
						glm::radians(rotationDelta.z),
						glm::vec3(0, 0, 1)
					);
					
					ImGui::Text("Scale");
					
					glm::vec3 scale = node->GlobalTransform().Scale();
		
					ImGui::InputFloat3("##Scale", &scale[0]);
	
					glm::vec3 scaleDelta = glm::zero<glm::vec3>();
	
					ImGui::SliderFloat3("##ScaleDelta", &scaleDelta[0], -1, 1);
	
					scale += scaleDelta;
		
					node->GlobalTransform().Scale() = scale;

					ImGui::TreePop();
				}

				std::string objectSectionHeader = std::format("Object count: {}", (int) node->AttachedObjects().size());

				if (ImGui::TreeNode(objectSectionHeader.c_str())) {

					int index = 0;
					for (GameObject* obj : node->AttachedObjects()) {
						ImGui::PushID(obj->GetID());

						if (ImGui::TreeNode(std::format("{}: {}", index, obj->GetName()).c_str())) {
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

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

int DebugInspector::Order() {
	return 1000;
}