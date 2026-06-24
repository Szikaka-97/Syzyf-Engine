#pragma once

#include <string>
#include <glm/fwd.hpp>
#include <imgui.h>

#include <SceneComponent.h>
#include <Scene.h>
#include <TypeInfo.h>

class ImGuiDrawable {
public:
	virtual void DrawImGui() = 0;
};

class DebugInspector : public SceneComponent {
public:
	DebugInspector(Scene* scene);

	virtual void DrawImGui();
	
	virtual int Order();
};

namespace Debug {
	template<typename T>
	bool Property(T& property, const std::string& name);
	
	template<>
	bool Property(float& property, const std::string& name);
	template<>
	bool Property(glm::vec2& property, const std::string& name);
	template<>
	bool Property(glm::vec3& property, const std::string& name);
	template<>
	bool Property(glm::vec4& property, const std::string& name);

	template<>
	bool Property(int& property, const std::string& name);
	template<>
	bool Property(glm::ivec2& property, const std::string& name);
	template<>
	bool Property(glm::ivec3& property, const std::string& name);
	template<>
	bool Property(glm::ivec4& property, const std::string& name);

	template<>
	bool Property(unsigned int& property, const std::string& name);
	template<>
	bool Property(glm::uvec2& property, const std::string& name);
	template<>
	bool Property(glm::uvec3& property, const std::string& name);
	template<>
	bool Property(glm::uvec4& property, const std::string& name);

	template<>
	bool Property(glm::mat3& property, const std::string& name);
	template<>
	bool Property(glm::mat4& property, const std::string& name);

	bool Property(SceneNode* owner, SceneNode*& property, const std::string& name);
	
	bool Property(SceneNode* owner, std::vector<SceneNode*>& property, const std::string& name);

	template<typename T>
		requires (std::derived_from<T, GameObject>)
	bool Property(SceneNode* owner, T*& property, const std::string& name);

	template<typename T>
		requires (std::derived_from<T, GameObject>)
	bool Property(SceneNode* owner, std::vector<T*>& property, const std::string& name);

	void RegisterGameObjectProperty(SceneNode* owner, GameObject** property);

	void RegisterGameObjectProperty(SceneNode* owner, std::vector<GameObject*>* property);

	void CheckDeletedNode(SceneNode* deleted);
	void CheckDeletedObject(GameObject* deleted);
};

template<typename T>
	requires (std::derived_from<T, GameObject>)
bool Debug::Property(SceneNode* owner, T*& property, const std::string& name) {
	std::string displayName = "";

	if (property) {
		displayName = std::format("{} ({})", property->GetNode()->GetName(), TypeInfo::GetTypeInfo(typeid(*property)).name);
	}

	ImGui::InputTextWithHint(name.c_str(), std::format("nullptr {}", TypeInfo::GetTypeInfo(typeid(T)).name).c_str(), displayName.data(), displayName.size(), ImGuiInputTextFlags_ReadOnly);

	if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		property = nullptr;
	}

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRAPH_SCENE_NODE")) {
			SceneNode* droppedNode = *(SceneNode**) payload->Data;

			auto objs = droppedNode->GetAllObjects<T>();
			
			for (auto* obj : objs) {
				if (property == obj) {
					continue;
				}

				property = obj;

				ImGui::EndDragDropTarget();

				RegisterGameObjectProperty(owner, (GameObject**) &property);
				
				return true;
			}
		}

		ImGui::EndDragDropTarget();
	}
	
	if (property) {
		ImGui::SameLine();

		ImGui::PushID(property->GetID());

		if (ImGui::Button("Delete")) {
			property = nullptr;
		}

		ImGui::PopID();
	}

	return false;
}

template<typename T>
	requires (std::derived_from<T, GameObject>)
bool Debug::Property(SceneNode* owner, std::vector<T*>& property, const std::string& name) {
	bool changed = false;

	if (ImGui::TreeNode(name.c_str())) {
		if (ImGui::Button("+")) {
			property.resize(property.size() + 1);
		}

		ImGui::SameLine();

		if (ImGui::Button("-")) {
			property.resize(property.size() - 1);
		}

		for (int i = 0; i < property.size(); i++) {
			std::string displayName = "";

			if (property[i]) {
				displayName = std::format("{} ({})", property[i]->GetNode()->GetName(), TypeInfo::GetTypeInfo(typeid(*property[i])).name);
			}

			if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
				property[i] = nullptr;
			}

			ImGui::PushID(i);

			ImGui::InputTextWithHint(name.c_str(), std::format("nullptr {}", TypeInfo::GetTypeInfo(typeid(T)).name).c_str(), displayName.data(), displayName.size(), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRAPH_SCENE_NODE")) {
					SceneNode* droppedNode = *(SceneNode**) payload->Data;

					auto objs = droppedNode->GetAllObjects<T>();
					
					for (auto* obj : objs) {
						if (property[i] == obj) {
							continue;
						}

						property[i] = obj;

						ImGui::EndDragDropTarget();

						changed = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
			
			if (property[i]) {
				ImGui::SameLine();

				if (ImGui::Button("Delete")) {
					property[i] = nullptr;
				}
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}
	
	if (changed) {
		RegisterGameObjectProperty(owner, (std::vector<GameObject*>*) &property);					
	}

	return changed;
}