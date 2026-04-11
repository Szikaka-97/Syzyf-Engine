#include "animation/AnimationComponent.h"
#include <Debug.h>

#include <imgui.h>

#include <Scene.h>

DebugInspector::DebugInspector(Scene* scene):
SceneComponent(scene) { }

void DrawNodeImGui(SceneNode* node) {
	ImGui::PushID(node->GetID());

	std::string treeHeader = node->GetName();

	if (treeHeader.empty()) {
		treeHeader = std::to_string(node->GetID());
	}

	if (ImGui::TreeNode(treeHeader.c_str())) {
		ImGui::Text("Node ID: %i", node->GetID());

		bool nodeEnabled = node->IsEnabled();

		ImGui::Checkbox("Enabled", &nodeEnabled);

		node->SetEnabled(nodeEnabled);

		if (ImGui::TreeNode("Layer")) {
			const float size = ImGui::CalcTextSize("00").x;

			for (int y = 0; y < 4; y++) {
				for (int x = 0; x < 8; x++) {
					if (x > 0) {
						ImGui::SameLine();
					}

					uint8_t layer = y * 8 + x;

					ImGui::PushID(layer);

					if (ImGui::Selectable(
						std::to_string(layer).c_str(),
						node->GetLayer() == layer,
						0,
						ImVec2(size, size)
					)) {
						node->SetLayer(layer);
					}

					ImGui::PopID();
				}
			}

			ImGui::TreePop();
		}

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

			if (glm::abs(scale.x) < 0.0001) {
				scale.x = 0.0001;
			}
			if (glm::abs(scale.y) < 0.0001) {
				scale.y = 0.0001;
			}
			if (glm::abs(scale.z) < 0.0001) {
				scale.z = 0.0001;
			}

			node->GlobalTransform().Scale() = scale;

			ImGui::TreePop();
		}

    AnimationComponent* animationComponent = node->GetObject<AnimationComponent>();
    if (animationComponent != nullptr) {
      if (ImGui::TreeNode("Animation")) {
        for (auto& animation : animationComponent->animations) {
          if (ImGui::TreeNode(animation.data.name.c_str())) {
            ImGui::Text("%s", std::format("Duration: {}", animation.data.duration).c_str());
            ImGui::Text("%s", std::format("Progress: {}", animation.timeActive).c_str());
            ImGui::Checkbox("Playing", &animation.playing);
            ImGui::Checkbox("Looping", &animation.looping);
            ImGui::DragFloat("Speed", &animation.speed, 1.0f, 0.0f, 5.0f, "%.2f");
            // animation.data.tracks.front().inputs add this maybe
            ImGui::TreePop();
          }
        }
        ImGui::TreePop();
      };
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

		auto children = node->GetChildren();

		if (ImGui::TreeNode(std::format("Child count: {}", children.size()).c_str())) {
			for (SceneNode* child : node->GetChildren()) {
				DrawNodeImGui(child);
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

void DebugInspector::DrawImGui() {
	SceneNode* treeRoot = GetScene()->GetRootNode();

	if (ImGui::TreeNode("GameObject Debug")) {
		DrawNodeImGui(treeRoot);

		ImGui::TreePop();
	}
}

int DebugInspector::Order() {
	return -1000;
}
