#include <Debug.h>

#include <glm/gtc/matrix_access.hpp>

#include <animation/AnimationComponent.h>
#include <Scene.h>

struct NodeCoupling {
	SceneNode* owner;
	SceneNode** value;
};

struct NodeVectorCoupling {
	SceneNode* owner;
	std::vector<SceneNode*>* value;
};

struct GameObjectCoupling {
	SceneNode* owner;
	GameObject** value;
};

struct GameObjectVectorCoupling {
	SceneNode* owner;
	std::vector<GameObject*>* value;
};

std::vector<NodeCoupling> registeredNodeCouplings;
std::vector<NodeVectorCoupling> registeredNodeVectorCouplings;
std::vector<GameObjectCoupling> registeredGameObjectCouplings;
std::vector<GameObjectVectorCoupling> registeredGameObjectVectorCouplings;

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

			glm::vec3 position = node->LocalTransform().Position();

			ImGui::InputFloat3("##Position", &position[0]);

			glm::vec3 positionDelta = glm::zero<glm::vec3>();

			ImGui::SliderFloat3("##PositionDelta", &positionDelta[0], -1, 1);

			position += positionDelta;

			ImGui::Text("Rotation");

			glm::vec3 rotationEuler = glm::degrees(glm::eulerAngles(node->LocalTransform().Rotation().Value()));

			ImGui::InputFloat3("##Rotation", &rotationEuler[0]);

			glm::vec3 rotationDelta = glm::zero<glm::vec3>();

			ImGui::SliderFloat3("##RotationDelta", &rotationDelta[0], -1, 1);

			ImGui::Text("Scale");
			
			glm::vec3 scale = node->LocalTransform().Scale();

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

			node->LocalTransform().Position() = position;
			node->LocalTransform().Rotation() = glm::quat(glm::radians(rotationEuler)) * glm::angleAxis(
				glm::radians(rotationDelta.x),
				glm::vec3(1, 0, 0)
			) * glm::angleAxis(
				glm::radians(rotationDelta.y),
				glm::vec3(0, 1, 0)
			) * glm::angleAxis(
				glm::radians(rotationDelta.z),
				glm::vec3(0, 0, 1)
			);
			node->LocalTransform().Scale() = scale;

			ImGui::TreePop();
		}

		AnimationComponent* animationComponent = node->GetObject<AnimationComponent>();
		if (animationComponent != nullptr && ImGui::TreeNode("Animation")) {
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
				ImGui::TreePop();
			}
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

template<>
bool Debug::Property(float& property, const std::string& name) {
	float mutValue = property;

	ImGui::InputFloat(name.c_str(), &property);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::vec2& property, const std::string& name) {
	glm::vec2 mutValue = property;

	ImGui::InputFloat2(name.c_str(), &property[0]);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::vec3& property, const std::string& name) {
	glm::vec3 mutValue = property;

	ImGui::InputFloat3(name.c_str(), &property[0]);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::vec4& property, const std::string& name) {
	glm::vec4 mutValue = property;

	ImGui::InputFloat4(name.c_str(), &property[0]);

	return property != mutValue;
}

template<>
bool Debug::Property(int& property, const std::string& name) {
	int mutValue = property;

	ImGui::InputInt(name.c_str(), &property);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::ivec2& property, const std::string& name) {
	glm::ivec2 mutValue = property;

	ImGui::InputInt2(name.c_str(), &property[0]);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::ivec3& property, const std::string& name) {
	glm::ivec3 mutValue = property;

	ImGui::InputInt3(name.c_str(), &property[0]);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::ivec4& property, const std::string& name) {
	glm::ivec4 mutValue = property;

	ImGui::InputInt4(name.c_str(), &property[0]);

	return property != mutValue;
}

template<>
bool Debug::Property(unsigned int& property, const std::string& name) {
	unsigned int mutValue = property;

	ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, &property, NULL, NULL, "%d", 0);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::uvec2& property, const std::string& name) {
	glm::uvec2 mutValue = property;

	ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &property, 2, NULL, NULL, "%d", 0);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::uvec3& property, const std::string& name) {
	glm::uvec3 mutValue = property;

	ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &property, 3, NULL, NULL, "%d", 0);

	return property != mutValue;
}
template<>
bool Debug::Property(glm::uvec4& property, const std::string& name) {
	glm::uvec4 mutValue = property;

	ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &property, 4, NULL, NULL, "%d", 0);

	return property != mutValue;
}

template<>
bool Debug::Property(glm::mat3& property, const std::string& name) {
	glm::mat3 origVal = property;

	glm::vec3 row0 = glm::row(property, 0);
	glm::vec3 row1 = glm::row(property, 1);
	glm::vec3 row2 = glm::row(property, 2);

	ImGui::InputFloat3(name.c_str(), &row0[0]);
	ImGui::InputFloat3(" ", &row1[0]);
	ImGui::InputFloat3("", &row2[0]);

	property[0][0] = row0[0];
	property[0][1] = row1[0];
	property[0][2] = row2[0];
	property[1][0] = row0[1];
	property[1][1] = row1[1];
	property[1][2] = row2[1];
	property[2][0] = row0[2];
	property[2][1] = row1[2];
	property[2][2] = row2[2];

	return origVal != property;
}
template<>
bool Debug::Property(glm::mat4& property, const std::string& name) {
	glm::mat4 origVal = property;

	glm::vec4 row0 = glm::row(property, 0);
	glm::vec4 row1 = glm::row(property, 1);
	glm::vec4 row2 = glm::row(property, 2);
	glm::vec4 row3 = glm::row(property, 3);

	ImGui::InputFloat4(name.c_str(), &row0[0]);
	ImGui::InputFloat4(" ", &row1[0]);
	ImGui::InputFloat4("", &row2[0]);
	ImGui::InputFloat4("  ", &row3[0]);

	property[0][0] = row0[0];
	property[0][1] = row1[0];
	property[0][2] = row2[0];
	property[0][3] = row3[0];
	property[1][0] = row0[1];
	property[1][1] = row1[1];
	property[1][2] = row2[1];
	property[1][3] = row3[1];
	property[2][0] = row0[2];
	property[2][1] = row1[2];
	property[2][2] = row2[2];
	property[2][3] = row3[2];
	property[3][0] = row0[3];
	property[3][1] = row1[3];
	property[3][2] = row2[3];
	property[3][3] = row3[3];

	return origVal != property;
}

bool Debug::Property(SceneNode* owner, SceneNode*& property, const std::string& name) {
	std::string displayName = "";

	if (property) {
		displayName = property->GetName();
	}

	if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		property = nullptr;
	}

	ImGui::InputTextWithHint(name.c_str(), "nullptr", displayName.data(), displayName.size(), ImGuiInputTextFlags_ReadOnly);

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRAPH_SCENE_NODE")) {
			SceneNode* droppedNode = *(SceneNode**) payload->Data;

			if (property != droppedNode) {
				property = droppedNode;

				registeredNodeCouplings.push_back({ owner, &property });

				ImGui::EndDragDropTarget();
				
				return true;
			}
		}

		ImGui::EndDragDropTarget();
	}

	return false;
}


bool Debug::Property(SceneNode* owner, std::vector<SceneNode*>& property, const std::string& name) {
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
				displayName = property[i]->GetName();
			}

			if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
				property[i] = nullptr;
			}

			ImGui::PushID(i);

			ImGui::InputTextWithHint(name.c_str(), "nullptr", displayName.data(), displayName.size(), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRAPH_SCENE_NODE")) {
					SceneNode* droppedNode = *(SceneNode**) payload->Data;

					property[i] = droppedNode;
					

				}

				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}
	
	if (changed) {
		registeredNodeVectorCouplings.push_back({ owner, &property });
	}

	return changed;
}

void Debug::RegisterGameObjectProperty(SceneNode* owner, GameObject** property) {
	registeredGameObjectCouplings.push_back({ owner, property });
}

void Debug::RegisterGameObjectProperty(SceneNode* owner, std::vector<GameObject*>* property) {
	registeredGameObjectVectorCouplings.push_back({ owner, property });
}

void Debug::CheckDeletedNode(SceneNode* deleted) {
	for (auto& coupling : registeredNodeCouplings) {
		if (*coupling.value == deleted) {
			*coupling.value = nullptr;
		}
	}

	for (auto& coupling : registeredGameObjectCouplings) {
		if ((*coupling.value)->GetNode() == deleted) {
			*coupling.value = nullptr;
		}
	}

	for (auto& coupling : registeredNodeVectorCouplings) {
		for (int i = 0; i < coupling.value->size(); i++) {
			if ((*coupling.value)[i] == deleted) {
				(*coupling.value)[i] = nullptr;
			}
		}
	}

	std::erase_if(registeredNodeCouplings, [deleted](auto& coupling) -> bool {
		return coupling.owner == deleted || *coupling.value == deleted;
	});

	std::erase_if(registeredNodeVectorCouplings, [deleted](auto& coupling) -> bool {
		return coupling.owner == deleted || std::any_of(coupling.value->begin(), coupling.value->end(), [deleted](auto val) -> bool {
			return val == deleted;
		});
	});

	std::erase_if(registeredGameObjectCouplings, [deleted](auto& coupling) -> bool {
		return coupling.owner == deleted;
	});

	std::erase_if(registeredGameObjectVectorCouplings, [deleted](auto& coupling) -> bool {
		return coupling.owner == deleted;
	});
}

void Debug::CheckDeletedObject(GameObject* deleted) {
	for (auto& coupling : registeredGameObjectCouplings) {
		if (*coupling.value == deleted) {
			*coupling.value = nullptr;
		}
	}

	for (auto& coupling : registeredGameObjectVectorCouplings) {
		for (auto& val : *coupling.value) {
			if (val == deleted) {
				val = nullptr;
			}
		}
	}

	std::erase_if(registeredGameObjectCouplings, [deleted](auto& coupling) -> bool {
		return *coupling.value == deleted;
	});

	std::erase_if(registeredGameObjectVectorCouplings, [deleted](auto& coupling) -> bool {
		return std::any_of(coupling.value->begin(), coupling.value->end(), [deleted](auto val) -> bool {
			return val == deleted;
		});
	});
}